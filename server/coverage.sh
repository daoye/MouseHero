#!/bin/bash

# YaYa Server 代码覆盖率检查脚本
set -e

echo "🚀 开始代码覆盖率检查..."

# 创建构建目录
BUILD_DIR="build"
# rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

echo "📦 配置CMake (启用覆盖率)..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCODE_COVERAGE=ON \
    -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0 -g" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"

echo "🔨 编译项目..."
make -j$(nproc)

echo "🧪 运行所有测试..."
ctest --output-on-failure --verbose

echo "📊 生成覆盖率报告..."

# 检查lcov是否安装
if ! command -v lcov &> /dev/null; then
    echo "❌ lcov未安装，正在安装..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y lcov
    elif command -v brew &> /dev/null; then
        brew install lcov
    else
        echo "❌ 无法自动安装lcov，请手动安装"
        exit 1
    fi
fi

# 初始化覆盖率数据
lcov --directory . --zerocounters

# 重新运行测试以收集覆盖率数据
echo "🔄 重新运行测试收集覆盖率数据..."
ctest --output-on-failure

# 收集覆盖率数据
echo "📈 收集覆盖率数据..."
lcov --directory . --capture --output-file coverage.info

# 过滤不需要的文件
echo "🧹 过滤系统文件和测试文件..."
lcov --remove coverage.info \
    '/usr/*' \
    '*/tests/*' \
    '*/external/*' \
    '*/build*/*' \
    '*/rs/target/*' \
    --output-file coverage_filtered.info \
    --ignore-errors unused

# 生成HTML报告
echo "📄 生成HTML覆盖率报告..."
genhtml coverage_filtered.info \
    --output-directory coverage_report \
    --title "YaYa Server Coverage" \
    --legend \
    --show-details \
    --branch-coverage \
    --function-coverage

# 显示覆盖率摘要
echo "📋 覆盖率摘要:"
lcov --summary coverage_filtered.info


echo "🎉 覆盖率检查完成!"
echo "📁 详细报告位置: file://$(pwd)/coverage_report/index.html"

cd .. 