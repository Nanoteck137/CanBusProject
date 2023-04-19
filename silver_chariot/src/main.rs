use std::io::{Cursor, Read, Write};
use std::net::TcpListener;
use std::sync::{Arc, RwLock};

use byteorder::ReadBytesExt;
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use speedwagon::{
    Packet, PacketType, RSNavState, ResponseErrorCode, Version, PACKET_START,
};

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

trait State: Sized + Sync + Send + Clone {
    fn set_control(&mut self, name: &str, val: bool) -> Option<()>;
    fn status(&self);
    fn list_controls(&self);
}

impl State for RSNavState {
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
}

fn write_response_packet<W>(
    writer: &mut W,
    pid: u8,
    error_code: ResponseErrorCode,
    data: &[u8],
) -> Option<()>
where
    W: Write,
{
    Packet::write_response(writer, pid, error_code, data).unwrap();
    writer.flush().unwrap();

    Some(())
}

fn handle_identify<W>(writer: &mut W, packet: &Packet) -> Option<()>
where
    W: Write,
{
    let mut data = Vec::new();

    let version = Version::new(2, 1, 1);
    data.extend_from_slice(&version.0.to_le_bytes());
    data.push(1); // NUM_CMDS

    let name = "Testing";
    data.push(name.len() as u8);
    data.extend_from_slice(name.as_bytes());

    write_response_packet(
        writer,
        packet.pid(),
        ResponseErrorCode::Success,
        &data,
    )?;

    Some(())
}

fn handle_status<W>(
    writer: &mut W,
    state: &Arc<RwLock<RSNavState>>,
    packet: &Packet,
) where
    W: Write,
{
    let mut cursor = Cursor::new([0; 16]);

    let state = { state.read().expect("Failed to get read lock").clone() };

    state.serialize(&mut cursor).unwrap();

    write_response_packet(
        writer,
        packet.pid(),
        ResponseErrorCode::Success,
        &cursor.into_inner(),
    );
}

fn handle_command<W>(
    writer: &mut W,
    state: &Arc<RwLock<RSNavState>>,
    packet: &Packet,
) -> Option<()>
where
    W: Write,
{
    let data = packet.data();
    let cmd_index = data[0];
    let params = &data[1..];

    match cmd_index {
        0x00 => {
            if params.len() < 1 {
                write_response_packet(
                    writer,
                    packet.pid(),
                    ResponseErrorCode::InsufficientFunctionParameters,
                    &[],
                )?;

                return Some(());
            }

            let mut state =
                state.write().expect("Failed to get write lock for state");
            state.led_bar_active = !state.led_bar_active;
            state.reverse_lights_active = !state.reverse_lights_active;

            write_response_packet(
                writer,
                packet.pid(),
                ResponseErrorCode::Success,
                &[],
            )?;
        }

        _ => write_response_packet(
            writer,
            packet.pid(),
            ResponseErrorCode::InvalidCommand,
            &[],
        )?,
    }

    Some(())
}

fn handle_ping<W>(writer: &mut W, packet: &Packet)
where
    W: Write,
{
    write_response_packet(
        writer,
        packet.pid(),
        ResponseErrorCode::Success,
        &[],
    );
}

fn handle_connection<S>(mut stream: S, state: &Arc<RwLock<RSNavState>>)
where
    S: Read + Write,
{
    loop {
        match stream.read_u8() {
            Ok(val) => {
                if val == PACKET_START {
                    let packet = Packet::read(&mut stream).unwrap();

                    match packet.typ() {
                        PacketType::Identify => {
                            handle_identify(&mut stream, &packet);
                        }
                        PacketType::Status => {
                            handle_status(&mut stream, state, &packet);
                        }
                        PacketType::Command => {
                            handle_command(&mut stream, state, &packet);
                        }
                        PacketType::Ping => handle_ping(&mut stream, &packet),
                        PacketType::Update | PacketType::Response => {
                            write_response_packet(
                                &mut stream,
                                packet.pid(),
                                ResponseErrorCode::InvalidPacketType,
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

    ToggleReverse,
    ToggleHighBeam,

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

        "togglereverse" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::ToggleReverse)
        }

        "togglehighbeam" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::ToggleHighBeam)
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

fn run_socket(state: Arc<RwLock<RSNavState>>) {
    let listener = TcpListener::bind("0.0.0.0:6969")
        .expect("Failed to bind TCP listener");

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("Got Connection");
                handle_connection(stream, &state);
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

fn run(state_lock: Arc<RwLock<RSNavState>>) {
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

                let mut state = {
                    state_lock.read().expect("Failed to get read lock").clone()
                };

                match cmd {
                    Command::SetControl { name, val } => {
                        state.set_control(&name, val).expect("No control");
                    }

                    Command::Status => {
                        state.status();
                    }

                    Command::RawStatus => {
                        let mut buf = Vec::new();
                        state.serialize(&mut buf).unwrap();
                        println!("{:#x?}", buf);
                    }

                    Command::ToggleReverse => {
                        state.reverse = !state.reverse;
                        state.reverse_camera = state.reverse;
                        if state.reverse_lights_active {
                            state.reverse_lights = state.reverse;
                        }
                    }

                    Command::ToggleHighBeam => {
                        state.high_beam = true;
                        if state.led_bar_active {
                            state.led_bar = true;
                        }
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

                *state_lock.write().expect("Failed to get write lock") = state;
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
    let state = Arc::new(RwLock::new(RSNavState::new()));

    let s = state.clone();
    std::thread::spawn(move || {
        run_socket(s);
    });

    run(state);
}
