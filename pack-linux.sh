#!/bin/bash
# ============================================================
#  pack-linux.sh — Package snake into an AppImage
# ============================================================

# ---- 可配置项 (可被环境变量覆盖) ----
AppImageToolPath="${APPIMAGETOOL_PATH:-${HOME}/appimagetool-x86_64.AppImage}"
# ------------------------------------------

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="$SCRIPT_DIR/dist-linux"
PACK_DIR="$SCRIPT_DIR/pack-linux"
APPDIR="$PACK_DIR/snake.AppDir"
OUTPUT="$PACK_DIR/snake-x86_64.AppImage"

# ---- 检查 dist ----
echo "==> 检查 dist-linux..."

[ -d "$DIST_DIR" ]       || { echo "错误: dist-linux/ 目录不存在！请先运行 build-linux.sh。"; exit 1; }
[ -f "$DIST_DIR/snake" ] || { echo "错误: dist-linux/snake 不存在！"; exit 1; }
[ -d "$DIST_DIR/resources" ] || { echo "错误: dist-linux/resources 不存在！"; exit 1; }

echo "  ✓ dist-linux/snake"
echo "  ✓ dist-linux/resources/"

# ---- 检查 AppImageTool ----
echo "==> 检查 AppImageTool..."

[ -f "$AppImageToolPath" ] || { echo "错误: AppImageTool 未找到: $AppImageToolPath"; exit 1; }
[ -x "$AppImageToolPath" ] || chmod +x "$AppImageToolPath"

echo "  ✓ $AppImageToolPath"

# ---- 构建 AppDir ----
echo "==> 构建 AppDir: $APPDIR"

rm -rf "$APPDIR"
mkdir -p "$APPDIR"

cp -r "$DIST_DIR/snake" "$DIST_DIR/resources" "$APPDIR/"

# 复制运行时依赖库（自动收集所有非系统 .so）
mkdir -p "$APPDIR/lib"

collect_libs() {
    ldd "$1" 2>/dev/null | grep -oP '/[^ ]+' | while read -r dep; do
        [ -f "$dep" ] || continue
        local name=$(basename "$dep")
        # 跳过已复制的和系统基础库
        [ -f "$APPDIR/lib/$name" ] && continue
        case "$name" in
            libc.so*|libm.so*|libpthread*|libdl.so*|librt.so*|ld-linux*|libstdc++*|libgcc_s*) continue ;;
        esac
        cp -L "$dep" "$APPDIR/lib/" && echo "  ✓ $name"
        collect_libs "$dep"
    done
}

collect_libs "$DIST_DIR/snake"

# .desktop
cat > "$APPDIR/snake.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=Snake
Comment=A classic snake game
Exec=snake
Icon=snake
Categories=Game;ArcadeGame;
Terminal=false
EOF

# 图标
if [ -f "$DIST_DIR/resources/img/apple.png" ]; then
    cp "$DIST_DIR/resources/img/apple.png" "$APPDIR/snake.png"
    echo "  ✓ 图标"
fi

# AppRun
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export APPDIR="${APPDIR:-$HERE}"
export LD_LIBRARY_PATH="${HERE}/lib:${LD_LIBRARY_PATH:-}"
# AppImage 运行时通常已设置 APPIMAGE；若缺失则尝试 ARGV0（同为 .AppImage 路径）
if [ -z "${APPIMAGE:-}" ] && [ -n "${ARGV0:-}" ]; then
    export APPIMAGE="$(readlink -f "${ARGV0}" 2>/dev/null || echo "${ARGV0}")"
fi
exec "${HERE}/snake" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo "  ✓ AppDir 就绪"

# ---- 打包 ----
echo "==> 打包 AppImage..."

ARCH="${ARCH:-x86_64}" "$AppImageToolPath" "$APPDIR" "$OUTPUT"

echo "==> 完成: $OUTPUT"
