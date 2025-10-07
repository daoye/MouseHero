include(FetchContent)

FetchContent_Declare(
  Corrosion
  GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
  GIT_TAG v0.5 # Optionally specify a commit hash, version tag or branch here
  UPDATE_DISCONNECTED true
  CMAKE_ARGS
  -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
  -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
