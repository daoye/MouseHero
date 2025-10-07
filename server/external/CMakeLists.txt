include(ExternalProject)

# libevent
ExternalProject_Add(
  libevent_project
  GIT_REPOSITORY https://github.com/libevent/libevent.git
  GIT_TAG release-2.1.12-stable
  PREFIX ${THIRDPARTY_SOURCE}/libevent
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
             -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_POLICY_VERSION_MINIMUM=3.5
             -DCMAKE_POSITION_INDEPENDENT_CODE=ON
             -DEVENT__LIBRARY_TYPE:STRING=STATIC
             -DEVENT__MSVC_STATIC_RUNTIME:BOOL=ON
             -DEVENT__DISABLE_TESTS=ON
             -DEVENT__DISABLE_SAMPLES:BOOL=ON
             -DEVENT__DISABLE_OPENSSL:BOOL=ON
  UPDATE_DISCONNECTED true
  PATCH_COMMAND git -C <SOURCE_DIR> apply
                ${CMAKE_CURRENT_SOURCE_DIR}/libevent.patch)
