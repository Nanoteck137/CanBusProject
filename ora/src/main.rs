use std::collections::HashMap;
use std::fs::File;
use std::io::{Read, Write};
use std::path::Path;
use std::process::Command;

use clap::{Parser, Subcommand};
use serde::{Deserialize, Serialize};
use tusk::DeviceType;

mod spec;

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

fn main() {
    let args = Args::parse();

    match args.action {
        Action::Firmware {} => {
            generate_tusk_bindings();

            let name = "the_world";
            let build_path = format!("build/{}", name);

            println!("---------------------------------------------");
            println!("Building firmware for device: '{}'", name);
            std::fs::create_dir_all(&build_path).unwrap();
            println!("Running cmake");
            generate_cmake_project("the_world", &build_path);
            println!();
            println!("Running make");
            compile_with_make(&build_path);
            println!("---------------------------------------------");
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
