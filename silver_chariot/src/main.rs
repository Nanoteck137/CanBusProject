use std::io::{Cursor, Read, Write};
use std::net::TcpListener;
use std::sync::RwLock;

use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use num_traits::ToPrimitive;
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use tusk::{ErrorCode, PacketType, PACKET_START};

static STATE: RwLock<State> = RwLock::new(State::new());

// NOTE(patrik):
//   Front
//   - LED BAR
//   - LED BAR (EXTRA LIGHT)
//   - HIGH BEAM
//   - IS LED BAR ACTIVE (SYNCED TO HIGH BEAM)
//   Back
//   - Reverse Camera
//   - Reverse Lights
//   - Trunk Lights
//   - Reverse
//   - Is Reverse Lights Active (SYNCED TO REVERSE)

#[derive(Clone, Default, Debug)]
struct State {
    led_bar: bool,
    led_bar_low_mode: bool,
    high_beam: bool,
    led_bar_active: bool,

    reverse_camera: bool,
    reverse_lights: bool,
    reverse: bool,
    reverse_lights_active: bool,
    trunk_lights: bool,
}

impl State {
    const fn new() -> Self {
        Self {
            led_bar: false,
            led_bar_low_mode: false,
            high_beam: false,
            led_bar_active: false,

            reverse_camera: false,
            reverse_lights: false,
            reverse: false,
            reverse_lights_active: false,
            trunk_lights: false,
        }
    }

    fn set_control(&mut self, name: &str, val: bool) -> Option<()> {
        match name {
            "led_bar" => self.led_bar = val,
            "led_bar_low_mode" => self.led_bar_low_mode = val,
            "high_beam" => self.high_beam = val,
            "led_bar_active" => self.led_bar_active = val,

            "reverse_camera" => self.reverse_camera = val,
            "reverse_lights" => self.reverse_lights = val,
            "reverse" => self.reverse = val,
            "reverse_lights_active" => self.reverse_lights_active = val,
            "trunk_lights" => self.trunk_lights = val,

            _ => return None,
        };

        Some(())
    }

    fn status(&self) {
        println!("  led_bar = {}", self.led_bar);
        println!("  led_bar_low_mode = {}", self.led_bar_low_mode);
        println!("  high_beam = {}", self.high_beam);
        println!("  led_bar_active = {}", self.led_bar_active);

        println!("  reverse_camera = {}", self.reverse_camera);
        println!("  reverse_lights = {}", self.reverse_lights);
        println!("  reverse = {}", self.reverse);
        println!("  reverse_lights_active = {}", self.reverse_lights_active);
        println!("  trunk_lights = {}", self.trunk_lights);
    }

    fn list_controls(&self) {
        println!("  led_bar");
        println!("  led_bar_low_mode");
        println!("  high_beam");
        println!("  led_bar_active");

        println!("  reverse_camera");
        println!("  reverse_lights");
        println!("  reverse");
        println!("  reverse_lights_active");
        println!("  trunk_lights");
    }

    fn pack<W>(&self, writer: &mut W) -> std::io::Result<()>
    where
        W: Write,
    {
        let b = (self.led_bar as u8) << 0 |
            (self.led_bar_low_mode as u8) << 1 |
            (self.high_beam as u8) << 2 |
            (self.led_bar_active as u8) << 3;
        writer.write_u8(b)?;

        let b = (self.reverse_camera as u8) << 0 |
            (self.reverse_lights as u8) << 1 |
            (self.reverse as u8) << 2 |
            (self.reverse_lights_active as u8) << 3 |
            (self.trunk_lights as u8) << 4;
        writer.write_u8(b)?;
        Ok(())
    }
}

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
    writer.flush().unwrap();
    writer.write(data).ok()?;
    writer.write_u16::<LittleEndian>(0).ok()?;

    Some(())
}

fn make_version(major: u8, minor: u8, patch: u8) -> u16 {
    ((major & 0x3f) as u16) << 10 |
        ((minor & 0x3f) as u16) << 4 |
        (patch & 0xf) as u16
}

fn handle_identify<W>(writer: &mut W, packet: &Packet) -> Option<()>
where
    W: Write,
{
    let mut data = Vec::new();

    let version = make_version(2, 1, 1);
    data.extend_from_slice(&version.to_le_bytes());
    data.push(1); // NUM_CMDS

    let name = "Testing";
    data.push(name.len() as u8);
    data.extend_from_slice(name.as_bytes());

    write_response_packet(writer, packet.pid, ErrorCode::Success, &data)?;

    Some(())
}

fn handle_status<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    let mut cursor = Cursor::new([0; 16]);

    let state = { STATE.read().expect("Failed to get read lock").clone() };

    state.pack(&mut cursor).unwrap();

    write_response_packet(
        writer,
        packet.pid,
        ErrorCode::Success,
        &cursor.into_inner(),
    );
}

fn handle_command<W>(writer: &mut W, packet: &Packet) -> Option<()>
where
    W: Write,
{
    let cmd_index = packet.data[0];
    let params = &packet.data[1..];

    match cmd_index {
        0x00 => {
            if params.len() < 1 {
                write_response_packet(
                    writer,
                    packet.pid,
                    ErrorCode::InsufficientFunctionParameters,
                    &[],
                )?;

                return Some(());
            }

            let mut state =
                STATE.write().expect("Failed to get write lock for state");
            // state.trunk_lamp = (params[0] & (1 << 0)) > 0;
            // state.backup_camera = (params[0] & (1 << 1)) > 0;
            // state.backup_lights = (params[0] & (1 << 2)) > 0;

            write_response_packet(
                writer,
                packet.pid,
                ErrorCode::Success,
                &[],
            )?;
        }

        _ => write_response_packet(
            writer,
            packet.pid,
            ErrorCode::InvalidCommand,
            &[],
        )?,
    }

    Some(())
}

fn handle_ping<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    write_response_packet(writer, packet.pid, ErrorCode::Success, &[]);
}

fn handle_connection<S>(mut stream: S)
where
    S: Read + Write,
{
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

            Err(_e) => {
                println!("Disconnect");
                return;
            }
        }
    }
}

#[derive(Debug)]
enum Command {
    SetControl { name: String, val: bool },
    Status,
    RawStatus,

    ListCommands,
    ListControls,

    Exit,
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

        "rawstatus" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::RawStatus)
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

        "exit" | "quit" | "q" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::Exit)
        }

        _ => None,
    }
}

fn run_socket() {
    let listener = TcpListener::bind("0.0.0.0:6969")
        .expect("Failed to bind TCP listener");

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("Got Connection");
                handle_connection(stream);
            }

            Err(_) => {}
        }
    }

    // let path = Path::new("/tmp/silver_chariot.sock");
    //
    // if path.exists() {
    //     std::fs::remove_file(&path).unwrap();
    // }
    //
    // let listener = UnixListener::bind(path).unwrap();
    //
    // for stream in listener.incoming() {
    //     match stream {
    //         Ok(stream) => {
    //             handle_connection(stream);
    //         }
    //
    //         Err(_) => {}
    //     }
    // }
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

                    Command::RawStatus => {
                        let mut buf = Vec::new();
                        state.pack(&mut buf).unwrap();
                        println!("{:#x?}", buf);
                    }

                    Command::ListCommands => {
                        println!(
                            "  control <name> <on/off/true/false> - Set \
                             Control state"
                        );
                        println!("  status - List current control status");
                        println!(
                            "  rawstatus - List current raw control status"
                        );
                        println!("  commands - List all commands");
                        println!("  controls - List all controls");
                    }

                    Command::ListControls => {
                        state.list_controls();
                    }

                    Command::Exit => {
                        break;
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
