use std::collections::HashMap;
use std::fs::File;
use std::io::{Read, Write};
use std::path::Path;
use std::process::Command;

use clap::{Parser, Subcommand};
use serde::{Deserialize, Serialize};
use tusk::DeviceType;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    action: Action,
}

#[derive(Subcommand, Debug)]
enum Action {
    /// Compile firmware
    Firmware {},
    TestFirmware {},
}

fn generate_gen_file<P>(output_dir: P, device_spec: &DeviceSpec)
where
    P: AsRef<Path>,
{
    let mut path = output_dir.as_ref().to_path_buf();
    path.push("gen.cpp");

    let mut file = File::create(path).unwrap();

    let name = &device_spec.name;
    let device_type = device_spec.device_type;

    let can_id = device_spec.can_id;

    let spaces = (0..32).map(|_| ' ').collect::<String>();

    let mut s = String::new();
    s += "{\n";
    device_spec.devices.iter().enumerate().for_each(|(i, d)| {
        s += format!("{}{{", &spaces[0..8]).as_str();
        s += format!(" .index = {},", i).as_str();
        s += format!(" .can_id = {:#x},", d.can_id).as_str();
        s += " },\n";
    });
    s += "    }";

    let num_controls = device_spec.controls.len();
    let num_lines = device_spec.lines.len();

    let mut controls = String::new();
    controls += "{ ";
    device_spec.controls.iter().for_each(|control| {
        controls += format!("{}, ", control).as_str();
    });
    controls += "}";

    let mut lines = String::new();
    lines += "{ ";
    device_spec.lines.iter().for_each(|line| {
        lines += format!("{}, ", line).as_str();
    });
    lines += "}";

    let s = format!(
        r#"
#include "common.h"

Config config = {{
    .name = "{name}",
    .version = MAKE_VERSION(0, 1, 0),
    .type = DeviceType::{device_type:?},

    .can_id = {can_id:#x},

    .devices = {s},

    .num_controls = {num_controls},
    .controls = {controls},

    .num_lines = {num_lines},
    .lines = {lines},
}};
    "#
    );

    println!("Spec: {s}");

    file.write_all(s.as_bytes()).unwrap();
}

fn generate_cmake_project<P>(path: P, target: P)
where
    P: AsRef<Path>,
{
    let source = path.as_ref();
    let target = target.as_ref();

    // cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S source -B target
    let status = Command::new("cmake")
        .arg("-DCMAKE_EXPORT_COMPILE_COMMANDS=1")
        .arg("-S")
        .arg(source)
        .arg("-B")
        .arg(target)
        .arg("-G")
        .arg("Unix Makefiles")
        .status()
        .unwrap();
    if !status.success() {
        panic!(
            "Failed to execute CMake ({}): {:?}",
            status.code().unwrap_or_else(|| 0),
            source
        );
    }
}

fn compile_with_make<P>(path: P)
where
    P: AsRef<Path>,
{
    let path = path.as_ref();

    let status = Command::new("make")
        .arg("-j8")
        .arg("-C")
        .arg(path)
        .status()
        .unwrap();

    if !status.success() {
        panic!(
            "Failed to execute make ({}): {:?}",
            status.code().unwrap_or_else(|| 0),
            path
        );
    }
}

fn generate_tusk_bindings() {
    std::fs::create_dir_all("build/tusk").unwrap();
    let output = "build/tusk/tusk.h";

    let status = Command::new("cbindgen")
        .arg("tusk")
        .arg("-o")
        .arg(output)
        .status()
        .unwrap();
    if !status.success() {
        panic!(
            "Failed to generate tusk bindings ({})",
            status.code().unwrap_or_else(|| 0),
        );
    }
}

#[derive(Serialize, Deserialize, Debug)]
struct Sp {
    name: String,
    devices: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug)]
struct Line {
    name: String,
    pin: u32,
}

#[derive(Serialize, Deserialize, Debug)]
struct Control {
    name: String,
    pin: u32,
}

#[derive(Serialize, Deserialize, Debug)]
struct Ge {
    name: String,
    can_id: String,
    devices: Vec<String>,
    lines: Option<Vec<Line>>,
    controls: Option<Vec<Control>>,
}

#[derive(Serialize, Deserialize, Debug)]
struct Spec {
    #[serde(rename = "SP")]
    sp: Vec<Sp>,
    #[serde(rename = "GE")]
    ge: Vec<Ge>,
}

#[derive(Clone, Debug)]
struct ConnectedDevice {
    can_id: u32,
}

#[derive(Clone, Debug)]
struct DeviceSpec {
    name: String,
    device_type: DeviceType,
    can_id: u32,

    devices: Vec<ConnectedDevice>,

    lines: Vec<u32>,
    controls: Vec<u32>,
}

fn main() {
    let args = Args::parse();

    let s = {
        let mut file =
            File::open("spec.json").expect("Failed to open spec.json");

        let mut s = String::new();
        file.read_to_string(&mut s).unwrap();

        s
    };

    let spec = serde_json::from_str::<Spec>(&s).unwrap();
    println!("Spec: {:#?}", spec);

    let mut devices = HashMap::new();

    spec.sp.iter().for_each(|dev| {
        let spec = DeviceSpec {
            name: dev.name.clone(),
            device_type: DeviceType::StarPlatinum,
            can_id: 0,

            devices: Vec::new(),

            lines: Vec::new(),
            controls: Vec::new(),
        };

        if devices.get(&dev.name).is_some() {
            panic!("Duplicated device name: '{}'", dev.name);
        }

        devices.insert(dev.name.clone(), spec);
    });

    spec.ge.iter().for_each(|dev| {
        let mut lines = Vec::new();
        if let Some(l) = &dev.lines {
            l.iter().for_each(|line| lines.push(line.pin));
        }

        let mut controls = Vec::new();
        if let Some(c) = &dev.controls {
            c.iter().for_each(|control| controls.push(control.pin));
        }

        assert_eq!(&dev.can_id[0..2], "0x");
        // TODO(patrik): Remove unwrap
        let can_id = u32::from_str_radix(&dev.can_id[2..], 16).unwrap();

        let spec = DeviceSpec {
            name: dev.name.clone(),
            device_type: DeviceType::GoldExperience,
            can_id,

            devices: Vec::new(),

            lines,
            controls,
        };

        if devices.get(&dev.name).is_some() {
            panic!("Duplicated device name: '{}'", dev.name);
        }

        devices.insert(dev.name.clone(), spec);
    });

    // Update references to devices

    spec.sp.iter().for_each(|dev| {
        for name in &dev.devices {
            let can_id = devices.get(name).unwrap().can_id;
            let dev = devices.get_mut(&dev.name).unwrap();
            dev.devices.push(ConnectedDevice { can_id });
        }
    });

    spec.ge.iter().for_each(|dev| {
        for name in &dev.devices {
            let can_id = devices.get(name).unwrap().can_id;
            let dev = devices.get_mut(&dev.name).unwrap();
            dev.devices.push(ConnectedDevice { can_id });
        }
    });

    match args.action {
        Action::Firmware {} => {
            generate_tusk_bindings();

            for (_, device) in &devices {
                let name = &device.name;
                let build_path = format!("build/{}", name);

                println!("---------------------------------------------");
                println!("Building firmware for device: '{}'", name);
                std::fs::create_dir_all(&build_path).unwrap();
                println!("Generating gen.cpp");
                generate_gen_file(&build_path, device);
                println!("Running cmake");
                generate_cmake_project("the_world", &build_path);
                println!();
                println!("Running make");
                compile_with_make(&build_path);
                println!("---------------------------------------------");
            }
        }

        Action::TestFirmware {} => {
            println!("---------------------------------------------");
            std::fs::create_dir_all("build/the_hand").unwrap();
            println!("Running cmake");
            generate_cmake_project("the_hand", "build/the_hand");
            println!();
            println!("Running make");
            compile_with_make("build/the_hand");
            println!("---------------------------------------------");
        }
    }
}
