#![allow(dead_code)]

use std::fs::File;
use std::io::Read;

#[derive(Debug)]
struct Pin {
    name: String,
    pin: u32,
}

#[derive(Debug)]
enum MappingControlKind {
    Toggle,
    Active,
}

#[derive(Debug)]
struct MappingControl {
    kind: MappingControlKind,
    control: String,
}

#[derive(Debug)]
struct Mapping {
    lines: Vec<String>,
    controls: Vec<MappingControl>,
}

#[derive(Debug)]
struct Device {
    name: String,
    can_id: u32,
    devices: Vec<usize>,
}

#[derive(Debug)]
struct Context {
    lines: Vec<Pin>,
    controls: Vec<Pin>,
}

impl Context {
    fn new() -> Self {
        Self {
            lines: Vec::new(),
            controls: Vec::new(),
        }
    }

    fn add_line(pin: Pin) -> usize {
        0
    }

    fn add_control(pin: Pin) -> usize {
        0
    }
}

pub fn test() {
    let s = {
        let mut file =
            File::open("spec.json").expect("Failed to open spec.json");

        let mut s = String::new();
        file.read_to_string(&mut s).unwrap();

        s
    };

    let context = Context::new();

    let spec = serde_json::from_str::<serde_json::Value>(&s).unwrap();
    println!("Spec: {:#?}", spec);
}
