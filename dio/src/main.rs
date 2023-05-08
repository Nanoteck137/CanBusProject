use std::io::{ErrorKind, Read, Write};
use std::net::TcpStream;
use std::os::unix::net::UnixStream;

use byteorder::ReadBytesExt;
use clap::{Parser, Subcommand};
use speedwagon::{Identity, Packet, PacketType, PACKET_START};

#[derive(Debug)]
enum Command {
    Identify,
    Status,
    Command { cmd: u8, params: Vec<u8> },
}

fn parse_u8(s: &str) -> Option<u8> {
    if s.len() >= 2 {
        if &s[0..2] == "0b" {
            Some(u8::from_str_radix(&s[2..], 2).ok()?)
        } else if &s[0..2] == "0x" {
            Some(u8::from_str_radix(&s[2..], 16).ok()?)
        } else {
            Some(u8::from_str_radix(s, 10).ok()?)
        }
    } else {
        Some(u8::from_str_radix(s, 10).ok()?)
    }
}

fn parse_cmd(cmd: &str) -> Option<Command> {
    let mut split = cmd.split(' ');

    let cmd = split.next()?;
    let cmd = cmd.to_lowercase();
    let cmd = cmd.as_str();

    match cmd {
        "identify" => Some(Command::Identify),
        "status" => Some(Command::Status),
        "command" => {
            let cmd = split.next()?;
            let cmd = parse_u8(cmd)?;

            let mut params = Vec::new();
            for s in split.map(|s| parse_u8(s)) {
                params.push(s?);
            }

            Some(Command::Command { cmd, params })
        }

        _ => None,
    }
}

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    action: Action,
}

#[derive(Subcommand, Debug)]
enum Action {
    List,
    Debug {
        port: String,

        #[arg(short, long, default_value_t = 115200)]
        baudrate: u32,
    },

    // TODO(patrik): Better name?
    Run {
        port: String,

        #[arg(short, long, default_value_t = 115200)]
        baudrate: u32,

        cmd: String,
    },

    RunSock {
        path: String,
        cmd: String,
    },

    RunTcp {
        addr: String,
        cmd: String,
    },
}

fn run_debug_monitor(port: &String, baudrate: u32) {
    let mut port = serialport::new(port, baudrate).open().unwrap();

    let mut buf = [0; 1024];
    loop {
        match port.read(&mut buf) {
            Ok(n) => {
                std::io::stdout().write(&buf[..n]).unwrap();
            }

            Err(e) => {
                if e.kind() == ErrorKind::TimedOut {
                    continue;
                }
            }
        }
    }
}

fn run<P>(port: &mut P, cmd: &str)
where
    P: Read + Write,
{
    let cmd = parse_cmd(cmd).expect("Failed to parse command");

    match cmd {
        Command::Identify => {
            let packet = Packet::new(0, PacketType::Identify);
            packet.serialize(port).unwrap();

            let packet = Packet::deserialize(port).unwrap();
            match packet.typ() {
                PacketType::OnIdentify(identity) => {
                    println!("Identity: {:?}", identity);
                }

                _ => panic!("Got packet: {:#?}", packet),
            }
        }

        Command::Status => {
            let packet = Packet::new(0, PacketType::Status);
            packet.serialize(port).unwrap();

            let packet = Packet::deserialize(port).unwrap();
            match packet.typ() {
                PacketType::OnStatus(status) => {
                    println!("Status: {:?}", status);
                }

                _ => panic!("Got packet: {:#?}", packet),
            }
        }

        Command::Command { cmd, params } => {
            todo!();
            // let mut data = Vec::new();
            // data.push(cmd);
            // data.extend_from_slice(&params);
            //
            // send_packet(port, PacketType::Command, &data);
            // let packet = wait_for_packet(port);
            // match packet.response() {
            //     Ok(_data) => {}
            //     Err(error_code) => eprintln!("Error: {:?}", error_code),
            // }
        }
    }
}

fn main() {
    let args = Args::parse();

    match args.action {
        Action::List => {
            let ports = serialport::available_ports().unwrap();
            for port in ports {
                println!("{}", port.port_name);
            }

            return;
        }

        Action::Debug { port, baudrate } => run_debug_monitor(&port, baudrate),
        Action::Run {
            port,
            baudrate,
            cmd,
        } => {
            let mut port = serialport::new(port, baudrate).open().unwrap();
            run(&mut port, &cmd)
        }

        Action::RunSock { path, cmd } => {
            let mut sock = UnixStream::connect(path)
                .expect("Failed to connect to socket");
            run(&mut sock, &cmd);
        }

        Action::RunTcp { addr, cmd } => {
            let mut sock =
                TcpStream::connect(addr).expect("Failed to connect to TCP");
            run(&mut sock, &cmd);
        }
    }
}
