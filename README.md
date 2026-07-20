# Motion Stabilizer Linux

Motion Stabilizer Linux is a non-invasive visual stabilization overlay for reducing motion sickness in 3D games. It draws stable visual anchors on top of the desktop or game window without injecting code into the game, reading game memory, or modifying game files.

## 解决什么问题

很多 3D 游戏会因为镜头快速移动、视野晃动、FOV 不适或缺少舒适性选项导致晕动症。这个工具通过外部桌面叠加层提供稳定视觉参考：

- 屏幕边缘参考标记
- 中心准星/视觉焦点
- 悬浮时钟/固定文字锚点

这些参考点能帮助大脑获得稳定参照，缓解部分玩家在 3D 游戏中的眩晕和恶心感。

## 截图位置

你可以后续把截图放到这些路径：

| 内容 | 建议路径 |
|---|---|
| 设置窗口 | `docs/screenshots/linux-settings.png` |
| 游戏内效果 | `docs/screenshots/linux-game.png` |
| 边缘叠加 | `docs/screenshots/linux-edge-overlay.png` |
| 中心准星 | `docs/screenshots/linux-crosshair.png` |

最佳支持环境：

- Arch Linux
- KDE Plasma
- Wayland
- Qt 6
- LayerShellQt
- KGlobalAccel

## 兼容性结论

不能保证“所有 Linux 环境下载后都有完整体验”。Linux 桌面生态差异很大，尤其 Wayland 会限制普通应用创建全局置顶、点击穿透和全局快捷键。

当前兼容等级：

| 环境 | 状态 | 说明 |
|------|------|------|
| KDE Plasma Wayland | 最佳支持 | 使用 LayerShellQt + KGlobalAccel |
| KDE Plasma X11 | 支持 | 使用 Qt 透明置顶窗口 + XFixes 点击穿透，KGlobalAccel 快捷键 |
| 通用 X11 | 基础支持 | 需要 Qt6、x11、xfixes；全局快捷键依赖 KDE 库是否启用 |
| GNOME Wayland | 降级/不保证 | Wayland 安全策略通常不允许普通应用稳定做全局 Overlay 和全局快捷键 |
| wlroots/Sway/Hyprland | 需要单独适配 | 应优先研究 layer-shell/desktop portal 组合 |
| Steam Deck/Gamescope | 需要实测 | 取决于 Gamescope 外层/内层运行方式 |

构建时 KDE 相关能力是可选的：

```bash
cmake -S . -B /tmp/motion-stabilizer-linux-build \
  -DENABLE_LAYER_SHELL_QT=ON \
  -DENABLE_KGLOBALACCEL=ON \
  -DENABLE_X11_FALLBACK=ON
```

最小 Qt 构建也能通过，但功能会降级：

```bash
cmake -S . -B /tmp/motion-stabilizer-linux-minimal-build \
  -DENABLE_LAYER_SHELL_QT=OFF \
  -DENABLE_KGLOBALACCEL=OFF \
  -DENABLE_X11_FALLBACK=OFF
cmake --build /tmp/motion-stabilizer-linux-minimal-build -j
```

## 已实现

- Qt 6 Widgets 设置窗口。
- KDE Wayland layer-shell 透明叠加窗口。
- X11 fallback：Qt 透明置顶窗口 + XFixes 输入穿透。
- 边缘叠加：`Box`、`Dome`、`Flag`。
- 中心准星：`Cross`、`Circle`、`Diamond`。
- 悬浮时钟：位置、字号、颜色、透明度、秒显隐。
- 宽高比安全区域：16:9、21:9、4:3、5:4。
- 分屏模式：Vertical、Horizontal。
- 边缘叠加、准星和时钟的基础配置。
- JSON 配置读写。
- Profile 保存、加载、删除。
- 中英文界面切换。
- 系统托盘：显示设置窗口、隐藏设置窗口、退出。
- KDE 全局快捷键：
  - F1：开关边缘叠加。
  - F2：开关中心准星。
  - F3：开关悬浮时钟。
- CMake install 规则和 `.desktop` 启动器文件。
- 每个显示器都会创建独立 Overlay，屏幕热插拔时自动重建。

## 构建

```bash
cmake -S . -B /tmp/motion-stabilizer-linux-build
cmake --build /tmp/motion-stabilizer-linux-build -j
```

## 环境检查

```bash
bash scripts/check-env.sh
```

## 运行

```bash
/tmp/motion-stabilizer-linux-build/motion-stabilizer-linux
```

## 安装到用户目录

```bash
cmake --install /tmp/motion-stabilizer-linux-build --prefix ~/.local
```

安装后可从应用启动器中打开 `Motion Stabilizer Linux`，或直接运行：

```bash
~/.local/bin/motion-stabilizer-linux
```

## Arch 本地打包

```bash
mkdir -p /tmp/motion-stabilizer-pkg
cp packaging/PKGBUILD /tmp/motion-stabilizer-pkg/
cd /tmp/motion-stabilizer-pkg
makepkg -si
```

## Flatpak 素材

已提供基础 manifest：

```text
packaging/io.github.motionstabilizer.MotionStabilizer.yml
```

可在配置好 Flatpak KDE SDK 后使用：

```bash
flatpak-builder --user --install --force-clean /tmp/motion-stabilizer-flatpak packaging/io.github.motionstabilizer.MotionStabilizer.yml
```

## AppImage 素材

准备 AppDir：

```bash
bash packaging/appimage-build.sh
```

生成 AppDir 后可用 `appimagetool` 创建 AppImage：

```bash
appimagetool /tmp/MotionStabilizerLinux.AppDir
```

当前 AppImage 脚本偏向本机打包骨架，仍依赖宿主 KDE/Qt Wayland 集成库；后续若要分发给其他发行版，需要接入 linuxdeploy/linuxdeployqt。

## 诊断模式

```bash
/tmp/motion-stabilizer-linux-build/motion-stabilizer-linux --diagnose
```

它会输出会话类型、Qt 平台、配置路径、屏幕列表、DPI/刷新率和关键 KDE runtime 文件状态。

## 配置位置

Qt 会根据应用名和组织名写入用户配置目录。当前路径通常为：

```text
~/.config/MotionStabilizer/motion-stabilizer-linux/config.json
```

Profile 目录通常为：

```text
~/.config/MotionStabilizer/motion-stabilizer-linux/profiles/
```

## 当前限制

- KDE Plasma Wayland 是最佳支持目标；其他桌面环境会按能力降级。
- 悬浮时钟当前通过设置面板调位置，尚未实现点击拖动。
- Wayland 点击穿透依赖 compositor 对 `Qt::WindowTransparentForInput` 和 layer-shell 的支持，需要在真实游戏窗口上继续验证。

真实游戏、Steam、Gamescope 和缩放比例测试步骤见：

```text
VALIDATION.md
```

跨桌面环境兼容性说明见：

```text
COMPATIBILITY.md
```
