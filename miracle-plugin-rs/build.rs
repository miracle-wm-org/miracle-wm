use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let manifest_path = PathBuf::from(&manifest_dir);
    let plugin_header = manifest_path.join("../miracle-wm-c/include/miracle/plugin.h");
    let bindings_file = manifest_path.join("src/bindings.rs");

    println!("cargo:rerun-if-changed={}", plugin_header.display());

    let target = env::var("TARGET").unwrap();

    if !target.contains("wasm") {
        // Allow non-wasm builds to succeed silently (e.g. during `cargo publish`)
        println!(
            "cargo:warning=miracle-plugin is only functional on wasm targets; skipping build script for target '{}'",
            target
        );
        return;
    }

    let mut builder = bindgen::Builder::default()
        .header(plugin_header.to_str().unwrap())
        .header("/usr/include/mircore/mir_toolkit/events/enums.h")
        .clang_arg("-I/usr/include/mircore")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .allowlist_type("miracle_.*")
        .allowlist_type("Mir.*")
        .allowlist_function("miracle_.*")
        .allowlist_var("MIRACLE_.*")
        .allowlist_var("mir_.*")
        .derive_default(true)
        .derive_debug(true)
        // Disable layout tests as they fail for cross-compilation targets with different pointer sizes
        .layout_tests(false);

    // Use C mode with wasm32 target to avoid C++ header requirements
    // The plugin.h header is designed to be C compatible via extern "C"
    builder = builder
        .clang_arg("--target=wasm32")
        .clang_arg("-xc")
        // Undefine __cplusplus so the header skips extern "C" blocks
        .clang_arg("-U__cplusplus");

    let bindings = builder.generate().expect("Unable to generate bindings");

    bindings
        .write_to_file(&bindings_file)
        .expect("Couldn't write bindings!");
}
