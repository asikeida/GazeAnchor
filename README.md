# GazeAnchor

GazeAnchor is a non-invasive visual stabilization overlay for reducing motion sickness in 3D games. It draws stable visual anchors on top of the desktop or game window without injecting code into the game, reading game memory, or modifying game files.

Current version: `0.1.0`

版本单一来源当前定义在 `CMakeLists.txt` 的 `project(GazeAnchor VERSION ...)`。发布脚本通过 `packaging/version.sh` 读取它，或在 release CI 中通过 `RELEASE_VERSION` 覆盖。

准备发新版本时，可先统一更新关键版本文件：

```bash
bash packaging/prepare-release-version.sh 0.2.0 2026-09-05
```

它会更新：`CMakeLists.txt`、`README.md` 顶部版本、AppStream release 节点、`packaging/gaze-anchor.spec` 和 `debian/changelog`。

## 解决什么问题

很多 3D 游戏会因为镜头快速移动、视野晃动、FOV 不适或缺少舒适性选项导致晕动症。这个工具通过外部桌面叠加层提供稳定视觉参考：

- 屏幕边缘参考标记
- 中心准星/视觉焦点
- 悬浮时钟/固定文字锚点

这些参考点能帮助大脑获得稳定参照，缓解部分玩家在 3D 游戏中的眩晕和恶心感。

## 截图位置


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
cmake -S . -B /tmp/gaze-anchor-build \
  -DBUILD_PROFILE=FULL \
  -DENABLE_LAYER_SHELL_QT=ON \
  -DENABLE_KGLOBALACCEL=ON \
  -DENABLE_X11_FALLBACK=ON
```

最小 Qt 构建也能通过，但功能会降级：

```bash
cmake -S . -B /tmp/gaze-anchor-minimal-build \
  -DBUILD_PROFILE=MINIMAL \
  -DENABLE_LAYER_SHELL_QT=OFF \
  -DENABLE_KGLOBALACCEL=OFF \
  -DENABLE_X11_FALLBACK=OFF
cmake --build /tmp/gaze-anchor-minimal-build -j
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
cmake -S . -B /tmp/gaze-anchor-build
cmake --build /tmp/gaze-anchor-build -j
```

## 环境检查

```bash
bash scripts/check-env.sh
```

## 运行

```bash
/tmp/gaze-anchor-build/gaze-anchor
```

如果程序已在运行，再次执行 `gaze-anchor` 会唤醒已有设置窗口，而不是启动第二个实例。

## 命令动作

```bash
gaze-anchor action show-settings
gaze-anchor action toggle-overlay
gaze-anchor action toggle-crosshair
gaze-anchor action toggle-clock
gaze-anchor action quit
```

这些命令可作为非 KDE 桌面环境的快捷键备用入口。

## 安装到用户目录

```bash
cmake --install /tmp/gaze-anchor-build --prefix ~/.local
```

安装后可从应用启动器中打开 `GazeAnchor`，或直接运行：

```bash
~/.local/bin/gaze-anchor
```

## Arch 本地打包

```bash
mkdir -p /tmp/gaze-anchor-pkg
cp packaging/PKGBUILD packaging/.SRCINFO /tmp/gaze-anchor-pkg/
cd /tmp/gaze-anchor-pkg
makepkg -si
```

当前 Arch 打包文件是 VCS 包形态：`gaze-anchor-git`。在仓库还没有固定 release tag 之前，这比伪装成固定版 `gaze-anchor` 更准确。

## Debian/Ubuntu deb 基础骨架

仓库根目录已提供 `debian/` 打包骨架，当前定位是首版起点，不是最终可发布的 Debian 包。

当前策略：

- 先构建 `BUILD_PROFILE=X11`
- 优先覆盖 Debian / Ubuntu / Mint 的基础原生安装路径
- 暂不把 KDE Wayland 专属依赖伪装成已经完成的正式 deb 支持

在安装了 Debian 打包工具链和对应 `-dev` 依赖后，可用典型命令构建：

```bash
bash packaging/build-deb-package.sh
```

等价的底层命令仍然是：`dpkg-buildpackage -us -uc -b`

默认情况下，生成的 `.deb`、`.changes` 和 `.buildinfo` 会出现在仓库父目录中。

构建后可进一步校验包内容：

```bash
bash packaging/verify-deb-package.sh
```

后续若要提供 KDE Wayland 完整功能的 deb，需要进一步确认目标发行版上的 LayerShellQt 和 KF6GlobalAccel 开发包名称与可用版本。

## Fedora/openSUSE RPM 基础骨架

仓库已提供 `packaging/gaze-anchor.spec`，当前同样是首版起点，不是最终可发布的 RPM 包。

当前策略：

- 先构建 `BUILD_PROFILE=X11`
- 优先覆盖 Fedora / openSUSE 的基础原生安装路径
- 暂不把 KDE Wayland 专属依赖伪装成已经完成的正式 rpm 支持

后续在具备 `rpmbuild` 环境和目标发行版依赖名确认后，可用该 spec 继续推进原生 rpm 打包。

本地辅助命令：

```bash
bash packaging/build-rpm-package.sh
```

该脚本会把当前工作树整理成临时 source tarball，再调用 `rpmbuild -ba`。

默认情况下，生成的 `.rpm` 和 `.src.rpm` 会出现在 `/tmp/opencode/gaze-anchor-rpmbuild/` 下。

构建后可进一步校验包内容：

```bash
bash packaging/verify-rpm-package.sh
```

## Release 产物整理

当 `.deb`、`.rpm`、AppImage 或 AppDir 已经在本地或 CI 中生成后，可用下面的脚本把它们统一整理到 `dist/release/`：

```bash
bash packaging/collect-release-artifacts.sh
```

它会：

- 收集 `.deb`、`.changes`、`.buildinfo`
- 收集 `.rpm`、`.src.rpm`
- 收集 `.AppImage`
- 如果存在 `/tmp/GazeAnchor.AppDir`，额外打成 `GazeAnchor.AppDir.tar.gz`
- 若源码 tarball 不存在，则自动生成 `source/gaze-anchor-<version>.tar.gz`
- 生成统一的 `SHA256SUMS`

也可以单独生成源码发布包：

```bash
bash packaging/build-source-tarball.sh
bash packaging/version.sh release-version
```

发布前可做一致性检查：

```bash
bash packaging/verify-release-consistency.sh
```

它会检查：

- 项目版本是否与预期一致
- tag 版本是否与预期一致
- `RELEASE-NOTES.md` 是否存在
- `SHA256SUMS` 是否存在
- 源码 tarball 是否存在
- `deb/`、`rpm/`、`appimage/` 中是否至少有一种交付产物
- 产物文件名是否包含预期版本
- `SHA256SUMS` 条目数是否覆盖全部 release 文件
- `sha256sum -c SHA256SUMS` 是否通过

## Tag 发布骨架

仓库已增加 `.github/workflows/release.yml`：

- 建议先运行 `bash packaging/prepare-release-version.sh <version> <date>`
- 当推送 `v*` tag 时触发
- 构建并收集源码 tarball
- 构建并校验 `.deb`
- 构建并校验 `.rpm`
- 构建 AppDir staging bundle
- 为收集到的产物生成 `SHA256SUMS`
- 自动创建 GitHub Release 并上传产物

当前限制：

- AppImage 仍未进入正式 release 产物；当前发布的是 `GazeAnchor.AppDir.tar.gz`
- 要发布真正可移植的 `.AppImage`，还需要把 `linuxdeployqt`/`appimagetool` 工具链接入 release 环境

构建后可进一步校验包内容：

```bash
bash packaging/verify-rpm-package.sh
```

## Flatpak 素材

已提供基础 manifest：

```text
packaging/io.github.gazeanchor.GazeAnchor.yml
```

可在配置好 Flatpak KDE SDK 后使用：

```bash
flatpak-builder --user --install --force-clean /tmp/gaze-anchor-flatpak packaging/io.github.gazeanchor.GazeAnchor.yml
```

当前 manifest 仍使用 `type: dir` 指向本地源码目录，适合本地开发和 beta 验证；要进入稳定分发，还需要改成固定 tag 或 release archive。

## AppImage 素材

准备 AppDir：

```bash
bash packaging/appimage-build.sh
```

脚本现在会：

- 使用 `BUILD_PROFILE=FULL` 做 Release 构建
- 安装到 `/tmp/GazeAnchor.AppDir`
- 校验 desktop 与 AppStream 元数据
- 记录当前 ELF 的直接依赖到 `ldd.txt`
- 在检测到 `linuxdeployqt` 或 `linuxdeployqt6` 时自动尝试收集运行库

若已安装 `appimagetool`，可直接生成 AppImage：

```bash
CREATE_APPIMAGE=1 bash packaging/appimage-build.sh
```

如果本机没有 `linuxdeployqt`/`linuxdeployqt6`，脚本会明确提示当前 AppDir 仍依赖宿主系统库，此时它还不是可分发的最终 AppImage。

## 诊断模式

```bash
/tmp/gaze-anchor-build/gaze-anchor --diagnose
```

它会输出版本、构建档位、Git commit、Qt 插件路径、配置路径、屏幕列表以及编译进程序的功能。

## 配置位置

Qt 会根据应用名和组织名写入用户配置目录。当前路径通常为：

```text
~/.config/GazeAnchor/gaze-anchor/config.json
```

Profile 目录通常为：

```text
~/.config/GazeAnchor/gaze-anchor/profiles/
```

首次运行时，如果检测到旧版本的 `~/.config/MotionStabilizer/motion-stabilizer-linux/`，会自动迁移已有配置和 profiles。

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
