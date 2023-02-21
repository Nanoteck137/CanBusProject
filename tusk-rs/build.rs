fn main() {
    println!("cargo:rerun-if-changed=../tusk.cpp");
    println!("cargo:rerun-if-changed=../tusk.h");

    cc::Build::new()
        .cpp(true)
        .file("../tusk/tusk.cpp")
        .compile("libtusk.a");

    let bindings = bindgen::Builder::default()
        .clang_args(["-x", "c++"])
        .header("wrapper.h")
        .generate()
        .expect("Failed to generate bindings");

    let out_path = std::path::PathBuf::from(std::env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Failed to write the generated bindings");
}
