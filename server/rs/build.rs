extern crate cbindgen;

use std::env;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::C)
        .with_pragma_once(true)
        .with_autogen_warning("// It's auto generate code DONT MODIFY THIS FILE")
        .with_include_version(true)
        .with_braces(cbindgen::Braces::NextLine)
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("target/include/rs.h");
}
