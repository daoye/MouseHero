cmake_minimum_required(VERSION 3.25)

project(
  core
  LANGUAGES C
  VERSION 1.0)

set(CMAKE_C_STANDARD 17)
set(THIRDPARTY_PREFIX ${CMAKE_BINARY_DIR}/thirdparty)
set(THIRDPARTY_SOURCE ${CMAKE_BINARY_DIR}/thirdparty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
else()
  add_definitions(-DRELEASE)
endif()

add_subdirectory(external)

add_subdirectory(rs)


include_directories(
  ${CMAKE_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/rs/target/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/mpack
  ${CMAKE_CURRENT_SOURCE_DIR}/src/input)

link_directories(${CMAKE_BINARY_DIR}/lib)

# 添加源文件
file(GLOB SERVER_SRC 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
list(REMOVE_ITEM SERVER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/run.c")

# 添加 input 模块源文件
file(GLOB INPUT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/input/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/input/keyboard/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/input/backend/*.c"
)
list(APPEND SERVER_SRC ${INPUT_SRC})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DUSE_UINPUT)
    
    # Detect and link libxkbcommon for character→key mapping
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(XKBCOMMON QUIET xkbcommon)
        if(XKBCOMMON_FOUND)
            add_definitions(-DHAVE_LIBXKBCOMMON)
            include_directories(${XKBCOMMON_INCLUDE_DIRS})
            link_directories(${XKBCOMMON_LIBRARY_DIRS})
            message(STATUS "Found libxkbcommon: ${XKBCOMMON_VERSION}")
        else()
            message(FATAL_ERROR "libxkbcommon not found; it is required for character→key mapping")
        endif()

        # Detect libxkbcommon-x11 for X11 layout detection (required)
        pkg_check_modules(XKBCOMMON_X11 QUIET xkbcommon-x11)
        if(XKBCOMMON_X11_FOUND)
            add_definitions(-DHAVE_LIBXKBCOMMON_X11)
            include_directories(${XKBCOMMON_X11_INCLUDE_DIRS})
            link_directories(${XKBCOMMON_X11_LIBRARY_DIRS})
            message(STATUS "Found libxkbcommon-x11: ${XKBCOMMON_X11_VERSION}")
        else()
            message(FATAL_ERROR "libxkbcommon-x11 not found; it is required for X11 layout detection")
        endif()
    endif()
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/mpack  MPACK_SRC)

# 创建核心库
add_library(server_lib STATIC ${SERVER_SRC} ${MPACK_SRC})
add_dependencies(server_lib rs libevent_project)
set_target_properties(server_lib PROPERTIES LINKER_LANGUAGE C)

if(CMAKE_SYSTEM STREQUAL "Windows")
  target_link_libraries(server_lib PUBLIC event.a rs ws2_32 iphlpapi pthread.a dbghelp runtimeobject propsys pathcch)
elseif(CMAKE_SYSTEM STREQUAL "Darwin")
  target_link_libraries(server_lib PUBLIC event.a rs pthread 
    "-framework CoreFoundation"
    "-framework CoreGraphics"
    "-framework ApplicationServices"
    "-framework Carbon"
    "-framework AppKit"
  )
else()
  set(LINUX_LIBS event.a rs pthread)
  if(XKBCOMMON_FOUND)
    list(APPEND LINUX_LIBS ${XKBCOMMON_LIBRARIES})
  endif()
  if(XKBCOMMON_X11_FOUND)
    list(APPEND LINUX_LIBS ${XKBCOMMON_X11_LIBRARIES} xcb)
  endif()
  target_link_libraries(server_lib PUBLIC ${LINUX_LIBS})
endif()


# 创建可执行文件
add_executable(server src/run.c)
target_link_libraries(server PRIVATE server_lib)

if(APPLE)
  set_target_properties(server PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  )
endif()


  # 启用测试
if (ENABLE_TEST)
  enable_testing()
  set(TEST_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

  add_library(server_lib_test STATIC ${SERVER_SRC} ${MPACK_SRC})
  add_dependencies(server_lib_test libevent_project)
  set_target_properties(server_lib_test PROPERTIES LINKER_LANGUAGE C)
  target_compile_definitions(server_lib_test PRIVATE YAYA_TESTS)

  if(CMAKE_SYSTEM STREQUAL "Windows")
    target_link_libraries(server_lib_test PUBLIC event.a ws2_32 iphlpapi pthread.a dbghelp)
  else()
    target_link_libraries(server_lib_test PUBLIC event.a pthread)
  endif()

  add_subdirectory(tests)
endif()
