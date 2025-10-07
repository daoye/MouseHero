# 设置代码覆盖率
if(CODE_COVERAGE)
    # 查找lcov
    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    # 添加覆盖率目标
    add_custom_target(coverage
        # 清理之前的覆盖率数据
        COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --zerocounters

        # 运行测试
        COMMAND ctest --output-on-failure

        # 收集覆盖率数据
        COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage.info
        
        # 过滤掉不需要的文件
        COMMAND ${LCOV} 
            --remove ${CMAKE_BINARY_DIR}/coverage.info 
            '/usr/*' 
            '*/tests/*' 
            '*/external/*' 
            --output-file ${CMAKE_BINARY_DIR}/coverage.info

        # 生成HTML报告
        COMMAND ${GENHTML} ${CMAKE_BINARY_DIR}/coverage.info 
            --output-directory ${CMAKE_BINARY_DIR}/coverage_report
            --title "YaYa Server Test Coverage"
            --legend
            --show-details
            --branch-coverage

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report"
    )
endif()

# 下载Unity测试框架
include(FetchContent)
FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# 设置测试源文件目录
set(TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 添加测试可执行文件
file(GLOB TEST_SOURCES "${TEST_SRC_DIR}/*_test.c")

# 为每个测试文件创建一个测试可执行文件
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIRECTORY})
    
    # 为测试添加编译选项
    if(CODE_COVERAGE)
        target_compile_options(${test_name} PRIVATE --coverage)
        target_link_options(${test_name} PRIVATE --coverage)
    endif()
    
    # ya_server_handler_test 使用测试专用库（不包含Rust依赖）
    if(test_name STREQUAL "ya_server_handler_test" OR test_name STREQUAL "ya_http_test")
        target_link_libraries(${test_name} PRIVATE unity server_lib_test)
    else()
        target_link_libraries(${test_name} PRIVATE unity server_lib)
    endif()
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach() 