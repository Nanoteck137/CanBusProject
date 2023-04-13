use std::io::{ErrorKind, Read, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::Path;
use std::sync::RwLock;

use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use num_traits::ToPrimitive;
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use tusk::{ErrorCode, PacketType, PACKET_START};

static STATE: RwLock<State> = RwLock::new(State::new());

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
    let mut data = [0; 16];

    let state = { STATE.read().expect("Failed to get read lock").clone() };

    data[0] = (state.backup_lights as u8) << 2 |
        (state.backup_camera as u8) << 1 |
        (state.trunk_lamp as u8) << 0;

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
    Status,

    ListCommands,
    ListControls,
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
                "on" | "true" => true,
                "off" | "false" => false,
                _ => return None,
            };

            Some(Command::SetControl { name, val })
        }

        "status" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::Status)
        }

        "commands" | "help" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::ListCommands)
        }

        "controls" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::ListControls)
        }

        _ => None,
    }
}

#[derive(Clone, Default, Debug)]
struct State {
    trunk_lamp: bool,
    backup_camera: bool,
    backup_lights: bool,
}

impl State {
    const fn new() -> Self {
        Self {
            trunk_lamp: false,
            backup_camera: false,
            backup_lights: false,
        }
    }

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

    fn status(&self) {
        println!("  trunk_lamp = {}", self.trunk_lamp);
        println!("  backup_camera = {}", self.backup_camera);
        println!("  backup_lights = {}", self.backup_lights);
    }

    fn list_controls(&self) {
        println!("  trunk_lamp");
        println!("  backup_camera");
        println!("  backup_lights");
    }
}

fn run_socket() {
    let path = Path::new("/tmp/silver_chariot.sock");

    if path.exists() {
        std::fs::remove_file(&path).unwrap();
    }

    let listener = UnixListener::bind(path).unwrap();

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                handle_connection(stream);
            }

            Err(_) => {}
        }
    }
}

fn run() {
    let mut rl = DefaultEditor::new().unwrap();

    if rl.load_history("history.txt").is_err() {
        println!("No previous history.");
    }

    loop {
        match rl.readline(">> ") {
            Ok(line) => {
                rl.add_history_entry(&line).unwrap();

                let cmd = if let Some(cmd) = parse_command(&line) {
                    cmd
                } else {
                    println!("Failed to parse command");
                    continue;
                };

                let mut state =
                    { STATE.read().expect("Failed to get read lock").clone() };

                match cmd {
                    Command::SetControl { name, val } => {
                        state.set_control(&name, val).expect("No control");
                    }

                    Command::Status => {
                        state.status();
                    }

                    Command::ListCommands => {
                        println!(
                            "  control <name> <on/off/true/false> - Set \
                             Control state"
                        );
                        println!("  status - List current control status");
                        println!("  commands - List all commands");
                        println!("  controls - List all controls");
                    }

                    Command::ListControls => {
                        state.list_controls();
                    }
                }

                *STATE.write().expect("Failed to get write lock") = state;
            }

            Err(ReadlineError::Interrupted) => {
                println!("CTRL-C");
                break;
            }

            Err(ReadlineError::Eof) => {
                println!("CTRL-D");
                break;
            }

            Err(e) => println!("Error: {:?}", e),
        }
    }

    rl.save_history("history.txt").unwrap();
}

fn main() {
    std::thread::spawn(move || {
        run_socket();
    });

    run();
}
