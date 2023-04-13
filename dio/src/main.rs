use std::io::{ErrorKind, Read, Write};
use std::os::unix::net::UnixStream;

use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use clap::{Parser, Subcommand};
use num_traits::{FromPrimitive, ToPrimitive};
use tusk::{DeviceType, ErrorCode, PacketType, PACKET_START};

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
}

#[derive(Debug)]
struct Packet {
    pid: u8,
    typ: PacketType,
    data: Vec<u8>,
    checksum: u16,
}

impl Packet {
    fn response(&self) -> Result<&[u8], ErrorCode> {
        if self.typ == PacketType::Response {
            let error_code = ErrorCode::from_u8(self.data[0]).unwrap();
            match error_code {
                ErrorCode::Success => Ok(&self.data[1..]),
                _ => Err(error_code),
            }
        } else {
            Err(ErrorCode::InvalidResponse)
        }
    }
}

fn get_next_packet<R>(reader: &mut R) -> Packet
where
    R: Read,
{
    let pid = reader.read_u8().unwrap();

    let typ = reader.read_u8().unwrap();
    let typ = PacketType::from_u8(typ).unwrap();

    let data_len = reader.read_u8().unwrap();

    let mut data = vec![0; data_len as usize];
    reader.read_exact(&mut data).unwrap();

    let checksum = reader.read_u16::<LittleEndian>().unwrap();

    return Packet {
        pid,
        typ,
        data,
        checksum,
    };
}

fn send_packet<W>(writer: &mut W, typ: PacketType, data: &[u8])
where
    W: Write,
{
    writer.write_u8(PACKET_START).unwrap();
    writer.write_u8(0).unwrap(); // PID
    writer.write_u8(typ.to_u8().unwrap()).unwrap();

    // TODO(patrik): Check data.len()
    writer.write_u8(data.len() as u8).unwrap();
    writer.write(data).unwrap();

    writer.write_u16::<LittleEndian>(0).unwrap();
}

fn send_empty_packet<W>(writer: &mut W, typ: PacketType)
where
    W: Write,
{
    send_packet(writer, typ, &[]);
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

fn wait_for_packet<R>(reader: &mut R) -> Packet
where
    R: Read,
{
    loop {
        if let Ok(val) = reader.read_u8() {
            if val == PACKET_START {
                return get_next_packet(reader);
            }
        }
    }
}

#[derive(Clone)]
#[repr(transparent)]
struct Version(u16);

impl Version {
    fn major(&self) -> u8 {
        ((self.0 >> 10) & 0x3f) as u8
    }

    fn minor(&self) -> u8 {
        ((self.0 >> 4) & 0x3f) as u8
    }

    fn patch(&self) -> u8 {
        ((self.0) & 0xf) as u8
    }
}

impl std::fmt::Debug for Version {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}.{}.{}", self.major(), self.minor(), self.patch())?;
        Ok(())
    }
}

#[derive(Clone, Debug)]
struct Identify {
    version: Version,
    num_cmds: usize,
    name: String,
}

impl Identify {
    fn parse(data: &[u8]) -> Option<Self> {
        if data.len() < 2 + 1 + 1 {
            return None;
        }

        let version = u16::from_le_bytes(data[0..2].try_into().ok()?);
        let num_cmds = data[2] as usize;
        let name_len = data[3] as usize;

        let data = &data[4..];

        if data.len() < name_len {
            return None;
        }

        let name = &data[0..name_len];
        let name = std::str::from_utf8(name).ok()?;

        Some(Identify {
            version: Version(version),
            num_cmds,
            name: name.to_string(),
        })
    }
}

fn run<P>(port: &mut P, cmd: &str)
where
    P: Read + Write,
{
    // send_empty_packet(port, PacketType::Identify);
    // let packet = wait_for_packet(port);
    // let info = match packet.response() {
    //     Ok(data) => {
    //         Identify::parse(data).expect("Failed to parse identify data")
    //     }
    //     Err(error_code) => {
    //         panic!("Failed to identify device: {:?}", error_code)
    //     }
    // };

    let cmd = parse_cmd(cmd).expect("Failed to parse command");

    match cmd {
        Command::Identify => {
            send_empty_packet(port, PacketType::Identify);
            let packet = wait_for_packet(port);
            match packet.response() {
                Ok(data) => println!("Identity: {:?}", Identify::parse(data)),
                Err(error_code) => eprintln!("Error: {:?}", error_code),
            }
        }

        Command::Status => {
            send_empty_packet(port, PacketType::Status);
            let packet = wait_for_packet(port);
            match packet.response() {
                Ok(data) => println!("Status: {:?}", data),
                Err(error_code) => eprintln!("Error: {:?}", error_code),
            }
        }

        Command::Command { cmd, params } => {
            let mut data = Vec::new();
            data.push(cmd);
            data.extend_from_slice(&params);

            send_packet(port, PacketType::Command, &data);
            let packet = wait_for_packet(port);
            match packet.response() {
                Ok(_data) => {}
                Err(error_code) => eprintln!("Error: {:?}", error_code),
            }
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
    }
}
