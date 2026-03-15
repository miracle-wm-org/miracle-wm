# miracle-plugin-rs
Rust bindings for Miracle's plugin system.

## Setup
```sh
sudo apt-get install -y libmircore-dev clang libclang-dev
rustup target add wasm32-wasip1
```

## Building
```sh
cargo build --target wasm32-wasip1 --release
```
