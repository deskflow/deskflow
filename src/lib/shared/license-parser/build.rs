use const_gen::*;
use ed25519::pkcs8::{DecodePublicKey, PublicKeyBytes};
use std::{env, fs, path::Path};

fn main() {
    let out_dir = env::var_os("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("publickey.rs");

    let public_key_file = fs::read_to_string("keys/public.pem").unwrap();
    let public_key: PublicKeyBytes = PublicKeyBytes::from_public_key_pem(&public_key_file).unwrap();

    let file_content = const_declaration!(PUBLIC_KEY = public_key.to_bytes());
    fs::write(&dest_path, file_content).unwrap();

    let _build = cxx_build::bridge("src/lib.rs");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/bridge.rs");
    println!("cargo:rerun-if-changed=keys/public.pem");
}
