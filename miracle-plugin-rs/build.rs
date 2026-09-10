fn main() {
    // `src/bindings.rs` is checked into the repository and shipped in the published crate, so
    // plugin authors need neither libclang nor Mir's development headers to build against this
    // SDK. Bindings are only regenerated when the `regen-bindings` feature is enabled:
    //
    //     cargo build --features regen-bindings
    //
    // Run that (and commit the result) whenever plugin.h changes.
    #[cfg(feature = "regen-bindings")]
    generate_bindings();
}

#[cfg(feature = "regen-bindings")]
fn generate_bindings() {
    use std::env;
    use std::path::PathBuf;

    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let manifest_path = PathBuf::from(&manifest_dir);
    let plugin_header = manifest_path.join("plugin.h");
    let bindings_file = manifest_path.join("src/bindings.rs");

    println!("cargo:rerun-if-changed={}", plugin_header.display());

    let builder = bindgen::Builder::default()
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
        .layout_tests(false)
        // Use C mode with wasm32 target to avoid C++ header requirements. The bindings always
        // describe the wasm ABI, whatever target we happen to be building for.
        .clang_arg("--target=wasm32")
        .clang_arg("-xc")
        // Undefine __cplusplus so the header skips extern "C" blocks
        .clang_arg("-U__cplusplus");

    let bindings = builder.generate().expect("Unable to generate bindings");

    bindings
        .write_to_file(&bindings_file)
        .expect("Couldn't write bindings!");
}
