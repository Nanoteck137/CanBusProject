use std::path::Path;
use std::process::Command;

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
    println!("Test: {:?}", status);
}

fn compile_with_make<P>(path: P)
where
    P: AsRef<Path>,
{
    let path = path.as_ref();

    let status = Command::new("make").arg("-C").arg(path).status().unwrap();
    println!("Status: {:?}", status);
}

fn main() {
    // TODO(patrik): Compile "the world"
    // TODO(patrik): Upload the world to a pico using picoprobe

    std::fs::create_dir_all("build/the_world").unwrap();
    generate_cmake_project("the_world", "build/the_world");
    compile_with_make("build/the_world");
}
