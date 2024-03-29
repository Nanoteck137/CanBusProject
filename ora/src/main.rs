use std::path::Path;
use std::process::Command;

use clap::{Parser, Subcommand};

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    action: Action,
}

#[derive(Subcommand, Debug)]
enum Action {
    /// Compile firmware
    GenBindings {},
    Firmware {
        device: String,
    },
    TestFirmware {},
}

fn generate_cmake_project<P>(path: P, target: P, name: &str)
where
    P: AsRef<Path>,
{
    let source = path.as_ref();
    let target = target.as_ref();

    let device_name = format!("-DDEVICE_NAME={}", name);

    // cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DDEVICE_NAME=rsnav -S source -B target
    let status = Command::new("cmake")
        .arg("-DCMAKE_EXPORT_COMPILE_COMMANDS=1")
        .arg(device_name)
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

fn generate_speedwagon_bindings() {
    std::fs::create_dir_all("target/speedwagon").unwrap();
    let output = "target/speedwagon/speedwagon.h";

    let status = Command::new("cbindgen")
        .arg("speedwagon")
        .arg("-o")
        .arg(output)
        .status()
        .unwrap();
    if !status.success() {
        panic!(
            "Failed to generate speedwagon bindings ({})",
            status.code().unwrap_or_else(|| 0),
        );
    }
}

fn main() {
    let args = Args::parse();

    match args.action {
        Action::GenBindings {} => {
            println!("Generating bindings");
            generate_speedwagon_bindings();
        }

        Action::Firmware { device } => {
            generate_speedwagon_bindings();

            let name = device;
            let build_path = format!("target/device/{}", name);

            println!("---------------------------------------------");
            println!("Building firmware for device: '{}'", name);
            std::fs::create_dir_all(&build_path).unwrap();

            println!("Running cmake");
            generate_cmake_project("the_world", &build_path, &name);
            println!();

            println!("Running make");
            compile_with_make(&build_path);

            println!("---------------------------------------------");
        }

        Action::TestFirmware {} => {
            println!("---------------------------------------------");
            std::fs::create_dir_all("build/the_hand").unwrap();
            println!("Running cmake");
            // generate_cmake_project("the_hand", "build/the_hand");
            println!();
            println!("Running make");
            compile_with_make("build/the_hand");
            println!("---------------------------------------------");
        }
    }
}
