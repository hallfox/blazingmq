use std::io;

fn main() -> io::Result<()> {
    println!("cargo:rerun-if-env-changed=PKG_CONFIG_PATH");

    let bmq = pkg_config::Config::new()
        .statik(true)
        .probe("bmq");

    if bmq.is_err() {
        println!("pkg_config failed for bmq: {:?}", bmq);
        std::process::exit(1);
    }

    cxx_build::bridge("src/bridge.rs")
        .file("src/bmq/bridge/session.cpp")
        .flag_if_supported("-std=c++17")
        .compile("libbmqrs.a");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bmq/bridge/session.h");
    println!("cargo:rerun-if-changed=src/bmq/bridge/session.cpp");
    println!("cargo:rerun-if-changed=src/bmq/bridge/errors.h");
    println!("cargo:rerun-if-changed=src/bmq/bridge/errors.cpp");
    Ok(())
}
