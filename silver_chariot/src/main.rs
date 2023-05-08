use std::io::{Cursor, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, RwLock};
use std::time::Duration;

use byteorder::ReadBytesExt;
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use speedwagon::{
    Identity, Packet, PacketType, RSNavState, ResponseCode, Version,
    NUM_STATUS_BYTES, PACKET_START,
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
    fn exec_cmd<W>(
        &mut self,
        writer: &mut W,
        packet: &Packet,
        cmd_index: u8,
        params: &[u8],
    ) -> Option<()>
    where
        W: Write;
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
        println!();
        println!("  reverse_camera = {}", self.reverse_camera);
        println!("  reverse_lights = {}", self.reverse_lights);
        println!("  reverse = {}", self.reverse);
        println!("  reverse_lights_active = {}", self.reverse_lights_active);
        println!("  trunk_lights = {}", self.trunk_lights);
    }
    fn exec_cmd<W>(
        &mut self,
        writer: &mut W,
        packet: &Packet,
        cmd_index: u8,
        params: &[u8],
    ) -> Option<()>
    where
        W: Write,
    {
        panic!();
    }

    //
    // fn exec_cmd<W>(
    //     &mut self,
    //     writer: &mut W,
    //     packet: &Packet,
    //     cmd_index: u8,
    //     params: &[u8],
    // ) -> Option<()>
    // where
    //     W: Write,
    // {
    //     macro_rules! expect_num_params {
    //         ($num:expr) => {
    //             if params.len() < $num {
    //                 write_response_packet(
    //                     writer,
    //                     packet.pid(),
    //                     ResponseCode::InsufficientFunctionParameters,
    //                     &[],
    //                 )?;
    //
    //                 return None;
    //             }
    //         };
    //     }
    //
    //     macro_rules! success {
    //         () => {
    //             write_response_packet(
    //                 writer,
    //                 packet.pid(),
    //                 ResponseCode::Success,
    //                 &[],
    //             )?;
    //
    //             return Some(());
    //         };
    //     }
    //
    //     match cmd_index {
    //         0x00 => {
    //             // SetLedBarActive
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.led_bar_active;
    //                 self.set_led_bar_active(on);
    //             } else {
    //                 self.set_led_bar_active(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         0x01 => {
    //             // SetLedBarLowMode
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.led_bar_low_mode;
    //                 self.set_led_bar_low_mode(on);
    //             } else {
    //                 self.set_led_bar_low_mode(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         0x02 => {
    //             // ForceLedBar
    //
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.led_bar;
    //                 self.force_led_bar(on);
    //             } else {
    //                 self.force_led_bar(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         0x03 => {
    //             // SetTrunkLights
    //
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.trunk_lights;
    //                 self.set_trunk_lights(on);
    //             } else {
    //                 self.set_trunk_lights(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         0x04 => {
    //             // SetReverseLightsActive
    //
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.reverse_lights_active;
    //                 self.set_reverse_lights_active(on);
    //             } else {
    //                 self.set_reverse_lights_active(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         0x05 => {
    //             // ForceReverseLights
    //
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.reverse_lights;
    //                 self.force_reverse_lights(on);
    //             } else {
    //                 self.force_reverse_lights(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         0x06 => {
    //             // * 0x06 - ForceReverseCamera
    //
    //             expect_num_params!(1);
    //
    //             let on = params[0] & (1 << 0) > 0;
    //             let toggle = params[0] & (1 << 1) > 0;
    //
    //             if toggle {
    //                 let on = !self.reverse_camera;
    //                 self.force_reverse_camera(on);
    //             } else {
    //                 self.force_reverse_camera(on);
    //             }
    //
    //             success!();
    //         }
    //
    //         _ => write_response_packet(
    //             writer,
    //             packet.pid(),
    //             ResponseCode::InvalidCommand,
    //             &[],
    //         ),
    //     }
    // }
}

// fn write_response_packet<W>(
//     writer: &mut W,
//     pid: u8,
//     error_code: ResponseCode,
//     data: &[u8],
// ) -> Option<()>
// where
//     W: Write,
// {
//     Packet::write_response(writer, pid, error_code, data).unwrap();
//     writer.flush().unwrap();
//
//     Some(())
// }
//
// fn handle_identify<W>(writer: &mut W, packet: &Packet) -> Option<()>
// where
//     W: Write,
// {
//     let mut data = Vec::new();
//
//     let version = Version::new(2, 1, 1);
//     data.extend_from_slice(&version.0.to_le_bytes());
//     data.push(1); // NUM_CMDS
//
//     let name = "Testing";
//     data.push(name.len() as u8);
//     data.extend_from_slice(name.as_bytes());
//
//     write_response_packet(writer, packet.pid(), ResponseCode::Success, &data)?;
//
//     Some(())
// }
//
// fn handle_status<W>(
//     writer: &mut W,
//     state: &Arc<RwLock<RSNavState>>,
//     packet: &Packet,
// ) where
//     W: Write,
// {
//     let mut cursor = Cursor::new([0; 16]);
//
//     let state = { state.read().expect("Failed to get read lock").clone() };
//
//     state.serialize(&mut cursor).unwrap();
//
//     write_response_packet(
//         writer,
//         packet.pid(),
//         ResponseCode::Success,
//         &cursor.into_inner(),
//     );
// }
//
// fn handle_command<W>(
//     writer: &mut W,
//     state: &Arc<RwLock<RSNavState>>,
//     packet: &Packet,
// ) -> Option<()>
// where
//     W: Write,
// {
//     let data = packet.data();
//     let cmd_index = data[0];
//     let params = &data[1..];
//
//     let mut state = state.write().expect("Failed to get write lock for state");
//
//     state.exec_cmd(writer, packet, cmd_index, params)
// }
//
// fn handle_ping<W>(writer: &mut W, packet: &Packet)
// where
//     W: Write,
// {
//     write_response_packet(writer, packet.pid(), ResponseCode::Success, &[]);
// }

static CONNECTED: AtomicBool = AtomicBool::new(false);

fn test<W>(writer: &mut W, status_time: u16)
where
    W: Write,
{
    loop {
        if CONNECTED.load(Ordering::SeqCst) {
            let status = [1, 2, 3, 4, 5, 6, 7, 8];
            Packet::new(0xff, PacketType::OnStatus(status))
                .serialize(writer)
                .unwrap();
        } else {
            break;
        }

        std::thread::sleep(Duration::from_millis(status_time as u64));
    }
    println!("Exiting Status thread");
}

fn handle_connection(mut stream: TcpStream, state: &Arc<RwLock<RSNavState>>) {
    loop {
        println!("Connected: {}", CONNECTED.load(Ordering::SeqCst));
        let packet = match Packet::deserialize(&mut stream) {
            Ok(packet) => packet,
            Err(_) => {
                CONNECTED.store(false, Ordering::SeqCst);
                break;
            }
        };
        println!("Packet: {:#?}", packet);

        match packet.typ() {
            PacketType::Connect {
                send_status,
                status_time,
            } => {
                if CONNECTED.load(Ordering::SeqCst) {
                    Packet::new(
                        packet.id(),
                        PacketType::Error {
                            code: ResponseCode::Unknown,
                        },
                    )
                    .serialize(&mut stream)
                    .unwrap();
                } else {
                    Packet::new(packet.id(), PacketType::OnConnect)
                        .serialize(&mut stream)
                        .unwrap();
                    CONNECTED.store(true, Ordering::SeqCst);

                    let status_time = *status_time;
                    if *send_status {
                        let mut s = stream.try_clone().unwrap();
                        std::thread::spawn(move || test(&mut s, status_time));
                    }
                }
            }

            PacketType::Disconnect => {
                CONNECTED.store(false, Ordering::SeqCst);
            }

            PacketType::Identify => {
                if !CONNECTED.load(Ordering::SeqCst) {
                    Packet::new(
                        packet.id(),
                        PacketType::Error {
                            code: ResponseCode::InvalidPacketType,
                        },
                    )
                    .serialize(&mut stream)
                    .unwrap();

                    return;
                }

                let identity = Identity {
                    name: "Testing".to_string(),
                    version: Version::new(0, 1, 0),
                    num_cmds: 1,
                };

                Packet::new(packet.id(), PacketType::OnIdentify(identity))
                    .serialize(&mut stream)
                    .unwrap()
            }

            // let lock = state.read().unwrap();
            // let status = [0; NUM_STATUS_BYTES];
            // let mut cursor = Cursor::new(status);
            // lock.serialize(&mut cursor).unwrap();
            //
            // Packet::new(packet.id(), PacketType::OnStatus(status))
            //     .serialize(&mut stream)
            //     .unwrap()
            PacketType::Error { .. } |
            PacketType::Cmd { .. } |
            PacketType::Status |
            PacketType::OnConnect |
            PacketType::OnCmd |
            PacketType::OnIdentify(_) |
            PacketType::OnStatus(_) => Packet::new(
                packet.id(),
                PacketType::Error {
                    code: ResponseCode::InvalidPacketType,
                },
            )
            .serialize(&mut stream)
            .unwrap(),
        }

        // match packet.typ() {
        //     PacketType::Identify => {
        //         handle_identify(&mut stream, &packet);
        //     }
        //     PacketType::Status => {
        //         handle_status(&mut stream, state, &packet);
        //     }
        //     PacketType::Command => {
        //         handle_command(&mut stream, state, &packet);
        //     }
        //     PacketType::Ping => handle_ping(&mut stream, &packet),
        //     PacketType::Update | PacketType::Response => {
        //         write_response_packet(
        //             &mut stream,
        //             packet.pid(),
        //             ResponseCode::InvalidPacketType,
        //             &[],
        //         );
        //     }
        // }
    }
}

#[derive(Debug)]
enum Command {
    SetControl { name: String, val: bool },
    Status,
    RawStatus,

    ToggleReverse,
    ToggleHighBeam,

    Help,
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

        "help" => {
            if params.len() != 0 {
                return None;
            }

            Some(Command::Help)
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
                        state.reverse(!state.reverse);
                    }

                    Command::ToggleHighBeam => {
                        state.high_beam(!state.high_beam);
                    }

                    Command::Help => {
                        println!(
                            "  control <name> <on/off/true/false> - Set \
                             Control state"
                        );
                        println!("  status - List current control status");
                        println!(
                            "  rawstatus - List current raw control status"
                        );
                        println!("  help - Help");
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
    let mut state = RSNavState::new();
    state.led_bar_active = true;
    state.led_bar_low_mode = true;
    state.reverse_lights_active = true;

    let state = Arc::new(RwLock::new(state));

    let s = state.clone();
    std::thread::spawn(move || {
        run_socket(s);
    });

    run(state);
}
