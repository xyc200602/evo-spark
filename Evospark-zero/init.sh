#!/bin/bash

# Evo-spark-zero 环境初始化脚本
# 用途：初始化开发环境和启动服务器（如果适用）

set -e  # 遇到错误立即退出

echo "========================================="
echo "  Evo-spark-zero 环境初始化"
echo "========================================="
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查任务文件
if [ ! -f "task.json" ]; then
    echo -e "${RED}错误: task.json 文件不存在！${NC}"
    exit 1
fi

echo -e "${GREEN}✓ 找到 task.json${NC}"
echo ""

# 解析任务数量
TASK_COUNT=$(grep -c '"id":' task.json || echo "0")
echo -e "当前待完成任务: ${YELLOW}$TASK_COUNT${NC} 个"
echo ""

# 检查并读取 progress.txt
if [ -f "progress.txt" ]; then
    echo -e "${GREEN}✓ 找到 progress.txt${NC}"
    echo ""
    echo "最近更新："
    echo "-----------------------------------"
    tail -n 10 progress.txt
    echo "-----------------------------------"
else
    echo -e "${YELLOW}⚠ progress.txt 不存在，将创建新文件${NC}"
    echo "" > progress.txt
fi

echo ""
echo "========================================="
echo "  环境检查"
echo "========================================="
echo ""

# 检查 ESP-IDF 环境
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}错误: ESP-IDF 环境未设置！${NC}"
    echo "请先运行："
    echo "  export IDF_PATH=/path/to/esp-idf-v5.4.3"
    echo "  \$IDF_PATH/export.sh"
    exit 1
fi

echo -e "${GREEN}✓ ESP-IDF 已设置${NC}"
echo "  ESP-IDF 路径: $IDF_PATH"
echo ""

# 检查 Git 仓库
if [ -d ".git" ]; then
    echo -e "${GREEN}✓ Git 仓库${NC}"
    BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
    COMMIT=$(git rev-parse --short HEAD 2>/dev/null)
    echo "  当前分支: $BRANCH"
    echo "  最新提交: $COMMIT"
else
    echo -e "${YELLOW}⚠ 非 Git 仓库${NC}"
fi

echo ""
echo "========================================="
echo "  开发命令提示"
echo "========================================="
echo ""
echo "选择下一个任务："
echo "  1. 阅读 task.json 和选择优先级最高的未完成任务"
echo "  2. 按照 CLAUDE.md 中的 Step 2-6 执行任务"
echo ""
echo "开发命令："
echo "  idf.py build              - 编译项目"
echo "  idf.py flash              - 烧录固件到设备"
echo "  idf.py monitor            - 监控设备日志"
echo "  idf.py -p COM5 monitor   - 监控 COM5 端口"
echo ""
echo "初始化完成！"
echo "========================================="
