use std::io::{ErrorKind, Read, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::Path;

use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use num_traits::ToPrimitive;
use tusk::{ErrorCode, PacketType, PACKET_START};

#[derive(Debug)]
struct Packet {
    pid: u8,
    typ: PacketType,
    data: Vec<u8>,
}

fn parse_packet(stream: &mut impl Read) -> Option<Packet> {
    let pid = stream.read_u8().ok()?;

    let typ = stream.read_u8().ok()?;
    let typ: PacketType = typ.try_into().ok()?;

    let data_len = stream.read_u8().ok()?;

    let mut data = vec![0u8; data_len as usize];
    stream.read_exact(&mut data).ok()?;

    let _checksum = stream.read_u16::<LittleEndian>();

    Some(Packet { pid, typ, data })
}

fn write_packet_header<W>(
    writer: &mut W,
    pid: u8,
    typ: PacketType,
) -> Option<()>
where
    W: Write,
{
    writer.write_u8(PACKET_START).ok()?;
    writer.write_u8(pid).ok()?;
    writer.write_u8(typ.to_u8()?).ok()?;

    Some(())
}

fn _write_packet<W>(
    writer: &mut W,
    pid: u8,
    typ: PacketType,
    data: &[u8],
) -> Option<()>
where
    W: Write,
{
    write_packet_header(writer, pid, typ);

    writer.write_u8(data.len() as u8).ok()?;
    writer.write(data).ok()?;
    writer.write_u16::<LittleEndian>(0).ok()?;

    Some(())
}

fn write_response_packet<W>(
    writer: &mut W,
    pid: u8,
    error_code: ErrorCode,
    data: &[u8],
) -> Option<()>
where
    W: Write,
{
    write_packet_header(writer, pid, PacketType::Response);

    let len = data.len() + 1;
    writer.write_u8(len as u8).ok()?;
    writer.write_u8(error_code.to_u8()?).ok()?;
    writer.write(data).ok()?;
    writer.write_u16::<LittleEndian>(0).ok()?;

    Some(())
}

fn handle_connection(mut stream: UnixStream) {
    loop {
        match stream.read_u8() {
            Ok(val) => {
                if val == PACKET_START {
                    let packet = parse_packet(&mut stream).unwrap();

                    match packet.typ {
                        PacketType::Ping => {
                            write_response_packet(
                                &mut stream,
                                packet.pid,
                                ErrorCode::Success,
                                &[],
                            );
                        }

                        _ => todo!(),
                    }
                }
            }

            Err(e) => {
                if e.kind() == ErrorKind::UnexpectedEof {
                    println!("Disconnecting???");
                    return;
                }
            }
        }
    }
}

fn main() {
    let path = Path::new("/tmp/silver_chariot.sock");

    if path.exists() {
        std::fs::remove_file(&path).unwrap();
    }

    let listener = UnixListener::bind(path).unwrap();

    loop {
        match listener.accept() {
            Ok((stream, _addr)) => handle_connection(stream),
            Err(e) => eprintln!("Error: {:?}", e),
        }
    }
}
