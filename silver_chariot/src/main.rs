use std::collections::HashMap;
use std::io::{BufRead, ErrorKind, Read, Write};
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

fn make_version(major: u8, minor: u8, patch: u8) -> u16 {
    ((major & 0x3f) as u16) << 10 |
        ((minor & 0x3f) as u16) << 4 |
        (patch & 0xf) as u16
}

fn handle_identify<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    let mut data = Vec::new();

    let version = make_version(2, 1, 1);
    data.extend_from_slice(&version.to_le_bytes());
    data.push(10); // NUM_CMDS

    let name = "Testing";
    data.push(name.len() as u8);
    data.extend_from_slice(name.as_bytes());

    write_response_packet(writer, packet.pid, ErrorCode::Success, &data);
}

fn handle_status<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    let data = [0; 16];
    write_response_packet(writer, packet.pid, ErrorCode::Success, &data);
}

fn handle_command<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    let cmd_index = packet.data[0];
    let params = &packet.data[1..];
    println!("Handle Command: {} -> {:?}", cmd_index, params);

    write_response_packet(writer, packet.pid, ErrorCode::InvalidCommand, &[]);
}

fn handle_ping<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    write_response_packet(writer, packet.pid, ErrorCode::Success, &[]);
}

fn handle_connection(mut stream: UnixStream) {
    loop {
        match stream.read_u8() {
            Ok(val) => {
                if val == PACKET_START {
                    let packet = parse_packet(&mut stream).unwrap();

                    match packet.typ {
                        PacketType::Identify => {
                            handle_identify(&mut stream, &packet);
                        }
                        PacketType::Status => {
                            handle_status(&mut stream, &packet);
                        }
                        PacketType::Command => {
                            handle_command(&mut stream, &packet);
                        }
                        PacketType::Ping => handle_ping(&mut stream, &packet),
                        PacketType::Update | PacketType::Response => {
                            write_response_packet(
                                &mut stream,
                                packet.pid,
                                ErrorCode::InvalidPacketType,
                                &[],
                            );
                        }
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

#[derive(Debug)]
enum Command {
    SetControl { name: String, val: bool },
}

fn parse_command(cmd: &str) -> Option<Command> {
    let mut split = cmd.split(' ');
    let cmd = split.next()?;
    let cmd = cmd.to_lowercase();
    let cmd = cmd.as_str();

    let params = split.collect::<Vec<&str>>();

    match cmd {
        "control" => {
            if params.len() < 2 {
                return None;
            }

            let name = params[0].to_string();
            let val = params[1];
            let val = match val {
                "on" => true,
                "off" => false,
                _ => return None,
            };

            Some(Command::SetControl { name, val })
        }
        _ => None,
    }
}

#[derive(Default, Debug)]
struct State {
    trunk_lamp: bool,
    backup_camera: bool,
    backup_lights: bool,
}

impl State {
    fn set_control(&mut self, name: &str, val: bool) -> Option<()> {
        match name {
            "trunk_lamp" => {
                self.trunk_lamp = val;
                Some(())
            }

            "backup_camera" => {
                self.backup_camera = val;
                Some(())
            }

            "backup_lights" => {
                self.backup_lights = val;
                Some(())
            }

            _ => None,
        }
    }
}

fn main() {
    let mut state = State::default();

    let stdin = std::io::stdin();

    let mut line = String::new();
    stdin.lock().read_line(&mut line).unwrap();

    let line = line.trim();
    println!("Line: {:?}", line.trim());

    let cmd = parse_command(line).expect("Failed to parse command");
    println!("Cmd: {:?}", cmd);

    match cmd {
        Command::SetControl { name, val } => {
            state.set_control(&name, val).expect("No control");
        }
    }

    println!("State: {:?}", state);

    // let path = Path::new("/tmp/silver_chariot.sock");
    //
    // if path.exists() {
    //     std::fs::remove_file(&path).unwrap();
    // }
    //
    // let listener = UnixListener::bind(path).unwrap();
    //
    // loop {
    //     match listener.accept() {
    //         Ok((stream, _addr)) => handle_connection(stream),
    //         Err(e) => eprintln!("Error: {:?}", e),
    //     }
    // }
}
