#!/bin/bash
# GitHub 推送脚本
# 使用方法: chmod +x push_to_github.sh && ./push_to_github.sh

echo "=== LaserTag 项目 GitHub 推送脚本 ==="
echo ""

# 检查Git仓库
if [ ! -d ".git" ]; then
    echo "❌ 错误: 当前目录不是Git仓库"
    exit 1
fi

# 检查是否有remote
if ! git remote | grep -q origin; then
    echo "📌 添加远程仓库..."
    git remote add origin https://github.com/ChangMuhan/LaserTag.git
fi

# 查看当前状态
echo "📊 当前Git状态:"
git status -s
echo ""

# 查看提交历史
echo "📝 最近的提交:"
git log --oneline -3
echo ""

# 确认推送
read -p "是否推送到GitHub? (y/n): " confirm
if [ "$confirm" != "y" ]; then
    echo "❌ 取消推送"
    exit 0
fi

# 推送到main分支
echo "🚀 推送到 GitHub..."
git branch -M main
git push -u origin main

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 推送成功!"
    echo "🔗 GitHub链接: https://github.com/ChangMuhan/LaserTag"
else
    echo ""
    echo "❌ 推送失败,请检查:"
    echo "1. 网络连接是否正常"
    echo "2. GitHub仓库是否存在"
    echo "3. 是否有推送权限"
    echo ""
    echo "💡 如需配置SSH密钥,运行:"
    echo "   ssh-keygen -t ed25519 -C \"your_email@example.com\""
    echo "   cat ~/.ssh/id_ed25519.pub"
    echo "   然后复制到 GitHub Settings → SSH Keys"
fi
