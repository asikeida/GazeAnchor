# GazeAnchor 主流 Linux 发行与交付方案

> 文档状态：基于当前仓库的实施规划  
> 编写日期：2026-09-04  
> 适用仓库：`GazeAnchor` 当前 C++20 / Qt 6 Linux 版本  
> 目标：将当前可在 Arch Linux / KDE Plasma 环境构建运行的应用，完善为可持续构建、安装、升级和维护，并能面向主流 Linux 发行版交付的软件。

## 1. 结论先行

当前项目已经是 Linux 原生应用，不需要再走参考文档中的“从 WPF/Windows 移植到 Linux”路线。仓库已有以下基础：

- C++20、Qt 6 Widgets 和 CMake 构建系统。
- KDE Wayland 的 LayerShellQt Overlay 后端。
- X11 + XFixes 点击穿透后端。
- KF6 KGlobalAccel 全局快捷键。
- 多显示器 Overlay、托盘、JSON 配置、Profile 和诊断模式。
- 初步的 Arch `PKGBUILD`、Flatpak manifest 和 AppImage staging 脚本。

当前状态仍属于“可开发、可本机构建的早期 Linux 应用”，还不是可面向主流发行版发布的软件。主要缺口不是重写程序，而是：

1. 统一产品名、应用 ID、包名、可执行文件名和上游地址。
2. 稳定不同桌面会话下的能力探测与降级行为。
3. 补齐图标、AppStream、许可证安装等桌面集成资源。
4. 让构建配置明确失败，避免悄悄产出功能残缺的同版本程序。
5. 建立自动测试、CI、干净环境安装测试和实机验证记录。
6. 将现有打包骨架完善为可复现的 AppImage、deb、rpm、AUR 和 Flatpak。
7. 从同一个 Git tag 自动生成、校验、签名和发布所有产物。

必须接受一个产品事实：**发行版可安装不等于桌面会话功能完整。** Ubuntu、Fedora、Debian、Arch 等是软件包和系统环境；X11、KDE Wayland、GNOME Wayland、wlroots 和 Gamescope 才直接决定 Overlay、点击穿透和全局快捷键是否可用。

推荐的第一版正式支持承诺：

| 等级 | 环境 | 建议承诺 |
|---|---|---|
| Tier 1 | KDE Plasma Wayland | 设置、静态 Overlay、点击穿透、多屏、KGlobalAccel 快捷键完整支持 |
| Tier 1 | KDE Plasma X11 | 设置、Overlay、XFixes 点击穿透、多屏、KGlobalAccel 快捷键完整支持 |
| Tier 2 | 通用 X11 | 设置、Overlay、点击穿透正式支持；快捷键在通用后端完成前按能力降级 |
| Experimental | Sway、Hyprland 等 wlroots Wayland | 待增加并验证专用 layer-shell 路径后再提升等级 |
| Limited | GNOME Wayland | 设置界面可运行，但不承诺可靠的全局 Overlay 和全局快捷键 |
| Experimental | SteamOS / Gamescope | Desktop Mode 与 Gaming Mode 分开实测，不提前承诺游戏内可见性 |

## 2. 当前项目结构和发行基线

### 2.1 关键目录

```text
GazeAnchor/
├── CMakeLists.txt
├── README.md
├── COMPATIBILITY.md
├── VALIDATION.md
├── LICENSE
├── src/
│   ├── main.cpp
│   ├── AppConfig.{h,cpp}
│   ├── OverlayWidget.{h,cpp}
│   └── SettingsWindow.{h,cpp}
├── packaging/
│   ├── PKGBUILD
│   ├── appimage-build.sh
│   ├── io.github.gazeanchor.GazeAnchor.yml
│   └── gaze-anchor.desktop.in
├── scripts/
│   └── check-env.sh
└── docs/screenshots/
```

仓库根目录目前还存在 `CMakeCache.txt`、`Makefile`、ELF 可执行文件、AUTOMOC 目录等本地构建产物。正式发行必须统一使用全新的 out-of-tree 构建目录，不能复用这些带绝对路径的缓存。

### 2.2 当前技术栈

| 模块 | 当前实现 | 发行影响 |
|---|---|---|
| 语言 | C++20 | 主流发行版均可构建 |
| 构建 | CMake 3.21+ | 适合原生包、Flatpak 和 AppImage |
| UI | Qt 6 Widgets | 需要部署 Qt 平台插件及运行库 |
| KDE Wayland Overlay | LayerShellQt | 完整包需提供匹配 Qt/KF6 ABI 的依赖 |
| 全局快捷键 | KF6 KGlobalAccel | KDE 可用，非 KDE 环境不能视为通用方案 |
| X11 点击穿透 | Xlib + XFixes | 可覆盖通用 X11，但包需显式声明依赖 |
| 配置 | JSON + `QStandardPaths` | 需补 schema、原子写入和迁移策略 |
| 安装 | GNUInstallDirs | 当前只安装 executable 和 desktop 文件 |

### 2.3 已有打包资产的实际成熟度

| 渠道 | 当前状态 | 主要问题 |
|---|---|---|
| Arch | 有 `PKGBUILD` 骨架 | 源码未固定、依赖不完整、URL 不一致、无 tag |
| AppImage | 只准备 AppDir | 未收集 Qt/KF6/LayerShellQt 库和 Qt plugins，不是真正便携包 |
| Flatpak | 本地开发 manifest | 使用本地目录 source，App ID 与 desktop 名称不一致，权限和配置路径未定型 |
| deb | 无 | 需要新增 Debian packaging 或统一生成方案 |
| rpm | 无 | 需要新增 Fedora/openSUSE spec 或统一生成方案 |
| CI/CD | 无 | 无 clean build、测试、打包和 Release 自动化 |

### 2.4 当前必须先处理的身份冲突

仓库目录名是 `GazeAnchor`，但项目内部仍使用：

- CMake 项目：`MotionStabilizerLinux`
- 可执行文件：`motion-stabilizer-linux`
- 桌面显示名：`Motion Stabilizer Linux`
- Flatpak ID：`io.github.gazeanchor.GazeAnchor`
- 配置组织名：`MotionStabilizer`
- `PKGBUILD` 中两个不同的 GitHub 地址

正式打包前必须作出一次产品决策：最终品牌是 **GazeAnchor**，还是继续使用 **Motion Stabilizer Linux**。以下方案默认最终采用 GazeAnchor，但执行时应先确认正式 GitHub namespace，再确定反向域名 App ID，例如 `io.github.<owner>.GazeAnchor`。

统一后应至少固定：

```text
产品显示名: GazeAnchor
可执行文件: gaze-anchor
原生包名: gaze-anchor
Desktop/AppStream/Flatpak ID: io.github.<owner>.GazeAnchor
配置目录: $XDG_CONFIG_HOME/gaze-anchor/
```

如果已有外部用户和配置，应提供一次性迁移；如果尚未正式发布，直接统一命名比长期保留兼容别名更简单。

## 3. 覆盖范围

### 3.1 首批发行版和产物

| 发行版家族 | 测试目标 | 主要产物 |
|---|---|---|
| Ubuntu | 最新两个受支持 LTS | AppImage、deb、Flatpak |
| Debian | 当前 stable | AppImage、deb、Flatpak |
| Linux Mint | 当前主版本 | 复用 Ubuntu deb，单独冒烟测试 |
| KDE Neon / Kubuntu | 当前稳定版 | 复用 Ubuntu deb，重点验证 KDE Wayland |
| Fedora | 当前版和前一版 | AppImage、rpm、Flatpak |
| openSUSE | Tumbleweed 和当前 Leap | AppImage、rpm、Flatpak |
| Arch Linux | rolling | AUR、AppImage、Flatpak |
| SteamOS | 当前稳定版 Desktop Mode | AppImage；Gaming Mode 保持 Experimental |

首发 CPU 架构建议只承诺 `x86_64`。`aarch64` 在构建链、依赖和实机验证稳定后进入第二阶段。Alpine/musl、32 位 x86 和无桌面的服务器发行版不属于首批范围。

### 3.2 “已覆盖”的完成定义

某个发行版只有同时满足以下条件，才能列入已支持列表：

- 用户不需要安装编译器、CMake 或开发包。
- 安装包能在该发行版的干净环境安装、启动、升级和卸载。
- desktop 文件、图标和 AppStream metadata 安装正确并通过校验。
- 设置、配置保存、Profile、退出路径和诊断功能可用。
- 应用能够识别 X11/Wayland、桌面环境和编译能力。
- 支持功能有测试记录；不支持功能会明确降级，而不是静默失败。
- Overlay 不抢焦点，支持环境中能够点击穿透。
- 升级和卸载不会意外删除用户配置。
- 发布页明确列出发行版、桌面、会话、GPU 和已知限制。

## 4. 发行前的软件工程改造

### 4.1 构建配置必须可预测

当前 `LayerShellQt`、`KF6GlobalAccel` 和 X11 依赖采用静默探测。同样的版本号可能在不同构建机上产出功能不同的二进制。建议定义明确构建档位：

| 构建档位 | 用途 | 必须依赖 |
|---|---|---|
| `FULL` | 官方原生包和完整 AppImage | Qt6、LayerShellQt、KF6GlobalAccel、X11、XFixes |
| `X11` | 不提供 KDE Wayland 后端的原生包 | Qt6、X11、XFixes；快捷键能力单独声明 |
| `MINIMAL` | 编译兼容性检查 | Qt6；不得作为功能完整正式包 |

官方构建在声明为 `FULL` 时，只要任一必需依赖缺失就应由 CMake 配置阶段失败。构建日志和 `--diagnose` 输出中应带版本、Git commit 和编译能力。

建议补充：

- `CMAKE_BUILD_TYPE=Release` 或多配置生成器的 Release 配置。
- 可选 LTO、strip 和 debug symbol 分离，不在源码构建时强制 strip。
- `BUILD_TESTING` 和 CTest。
- CPack 只作为 deb/rpm 起步工具；若需要严格发行版规范，可维护原生 `debian/` 和 `.spec`。
- Release 构建禁止复用仓库根目录缓存。

### 4.2 运行时能力探测和降级

当前程序只要发现 `XDG_SESSION_TYPE=wayland` 就设置 `QT_WAYLAND_SHELL_INTEGRATION=layer-shell`。这不能证明当前 compositor 支持该行为，尤其可能影响 GNOME Wayland。

需要改为：

1. 识别 Qt platform、桌面环境和实际可用后端。
2. 区分“编译进程序”“运行库存在”“当前 compositor 可用”三种状态。
3. 后端不可用时不创建假 Overlay，设置窗口应显示原因和建议。
4. GNOME Wayland 进入明确的受限模式，可提示使用 X11 会话，但不宣称完整支持。
5. `--diagnose` 不再硬编码 Arch 的 `/usr/lib` 路径，改为输出 Qt library paths、插件路径、D-Bus 服务状态和真实后端选择。

建议提供稳定的 CLI action，作为非 KDE 全局快捷键的通用备用方案：

```text
gaze-anchor action toggle-overlay
gaze-anchor action toggle-crosshair
gaze-anchor action toggle-clock
gaze-anchor action show-settings
gaze-anchor action quit
```

用户可在 GNOME、Sway、Hyprland 或其他桌面的系统快捷键设置中调用这些命令。后续再接入 `xdg-desktop-portal` GlobalShortcuts；不要把 KGlobalAccel 当作所有 Linux 桌面的通用接口。

### 4.3 生命周期和托盘

程序目前无条件创建 `QSystemTrayIcon`。发行前需要：

- 检测托盘是否可用。
- 在 GNOME 等无托盘环境中保留明确退出路径。
- 明确设置窗口关闭是退出、隐藏还是由用户配置，行为应稳定并有文档。
- 增加单实例机制，第二次启动时唤醒已有设置窗口，避免重复 Overlay。
- 可优先使用 D-Bus service/name，实现单实例和 CLI action。

### 4.4 配置数据

正式发布前应将配置写入完善为：

- 使用稳定的 XDG 路径。
- JSON 顶层增加 `schemaVersion`。
- 使用 `QSaveFile` 原子写入，避免 truncate 后异常退出损坏配置。
- 损坏配置自动备份并向用户说明已恢复默认值。
- 为字段新增、重命名和默认值变化编写迁移测试。
- 记录原生包、AppImage 和 Flatpak 之间的配置路径及迁移方法。

### 4.5 桌面集成资产

CMake install 需要统一安装：

```text
/usr/bin/gaze-anchor
/usr/share/applications/io.github.<owner>.GazeAnchor.desktop
/usr/share/metainfo/io.github.<owner>.GazeAnchor.metainfo.xml
/usr/share/icons/hicolor/scalable/apps/io.github.<owner>.GazeAnchor.svg
/usr/share/licenses/gaze-anchor/LICENSE
```

需要补齐：

- 自有 SVG 图标，以及必要尺寸的 PNG fallback。
- 与 App ID 同名的 desktop 文件和 AppStream metainfo。
- `Name`、`Comment`、`Exec`、`Icon`、`Categories`、`Keywords` 和本地化字段。
- AppStream 描述、项目 URL、发布版本、截图和内容评级。
- README 中真实截图，不再只保留建议路径。

校验命令应纳入 CI：

```bash
desktop-file-validate <desktop-file>
appstreamcli validate --pedantic <metainfo-file>
```

## 5. 分阶段实施过程

## 阶段 0：冻结身份和发布基线

目标：在继续打包前确定唯一产品身份和可重复基线。

任务：

- [ ] 决定使用 GazeAnchor 还是 Motion Stabilizer Linux 作为正式名称。
- [ ] 确定官方仓库 URL、维护者和反向域名 App ID。
- [ ] 统一 CMake project、target、desktop、AppStream、Flatpak、配置组织名和包名。
- [ ] 确定 SemVer 规则，并让 CMake 成为版本单一来源或由 Git tag 注入版本。
- [ ] 清理发行流程对根目录构建产物的依赖，完善 `.gitignore`。
- [ ] 修正文档中的旧路径，例如 `cmake -S linux` 和 `linux/scripts/check-env.sh`。
- [ ] 为当前版本建立一个干净 Release build 记录和功能基线。

验收门槛：

- 仓库内没有相互冲突的正式名称、URL 和 App ID。
- `cmake -S . -B build` 可从全新 checkout 构建。
- `gaze-anchor --version` 和 `--diagnose` 能输出版本、commit 和功能集合。

## 阶段 1：发行级应用基础

目标：让应用即使在能力受限的桌面环境中，也能稳定启动、解释状态并正常退出。

任务：

- [ ] 为后端建立明确的运行时能力状态和错误原因。
- [ ] 修正所有 Wayland 会话强制使用 layer-shell 的逻辑。
- [ ] 完善无托盘模式和关闭行为。
- [ ] 实现单实例和 CLI/D-Bus action。
- [ ] 将硬编码 F1/F2/F3 改为可配置，或至少处理冲突并提供备用 action。
- [ ] 使用 `QSaveFile` 原子保存配置。
- [ ] 增加配置 schema 和损坏恢复。
- [ ] 让诊断功能跨 Debian multiarch、Fedora `lib64` 和 Arch 路径工作。

验收门槛：

- KDE Wayland、KDE X11、GNOME Wayland 和无托盘环境都能启动、诊断和退出。
- 不支持 Overlay 的环境不会崩溃或静默失败。
- 第二实例不会创建第二组 Overlay。
- 配置写入中断不会破坏上一份有效配置。

## 阶段 2：测试和 CI 基础

目标：在开始维护多个包格式前，让软件行为和构建可自动验证。

优先增加的自动测试：

- [ ] AppConfig JSON round-trip、默认值、非法 JSON、schema 迁移。
- [ ] Profile 保存、加载、删除和非法文件名。
- [ ] 安全区域、分屏、准星和时钟几何计算。
- [ ] CLI 参数、版本、诊断和 action 路由。
- [ ] CMake install 后文件布局测试。
- [ ] desktop 和 AppStream 校验。
- [ ] Xvfb 下的 X11 启动 smoke test。
- [ ] 嵌套 KDE/Wayland 或可行的 compositor smoke test。

每次提交的 CI 建议：

| 作业 | 环境 | 内容 |
|---|---|---|
| Build full | Ubuntu/KDE 依赖容器 | clean configure、build、CTest |
| Build minimal | Ubuntu | 验证可选功能关闭时仍可编译 |
| Arch build | Arch 容器 | full build、依赖检查 |
| Fedora build | Fedora 容器 | full 或目标 profile 构建 |
| Static checks | Ubuntu | 编译警告、clang-tidy 可选、desktop/AppStream validation |
| X11 smoke | Xvfb | 启动、单实例、配置和退出 |
| Install smoke | 临时 DESTDIR | 检查安装布局和缺失文件 |

验收门槛：

- 每次提交至少完成 full/minimal clean build、单元测试和安装布局校验。
- Release 分支不允许忽略编译后端缺失。
- 自动产物能追溯到 commit。

## 阶段 3：桌面协议实机验证

目标：确认应用支持声明是实测结果，而不是仅凭构建成功推断。

测试维度：

| 维度 | 最低覆盖 |
|---|---|
| 会话 | KDE Wayland、KDE X11、通用 X11、GNOME Wayland、至少一个 wlroots、Gamescope |
| GPU | Intel、AMD、NVIDIA 各至少一套可获得环境 |
| 缩放 | 100%、125%、150%、200% |
| 屏幕 | 单屏、双屏、不同 DPR、负坐标、热插拔 |
| 游戏 | 原生游戏、Proton、窗口、无边框、独占全屏 |
| 生命周期 | 首次启动、第二实例、托盘缺失、桌面注销、异常退出 |

每条记录至少包含：

```text
应用版本/commit：
发行版和版本：
桌面和版本：
会话/Qt platform：
GPU/驱动：
屏幕布局和 DPR：
包格式：
游戏/Proton/Gamescope：
通过项：
失败项：
--diagnose 输出：
```

验收门槛：

- Tier 1 环境通过 Overlay 可见、置顶、不抢焦点、点击穿透、快捷键和多屏测试。
- GNOME Wayland 正确显示受限状态。
- wlroots 和 Gamescope 在通过实测前始终标记 Experimental。
- `VALIDATION.md` 从待办清单升级为带版本和结果的测试记录模板。

## 阶段 4：完成 AppImage

目标：先提供一个便于跨发行版测试的独立二进制产物。

现有 `appimage-build.sh` 只建立 AppDir，必须接入 `linuxdeploy`、适合 Qt 6 的插件或等效部署流程，并确认收集：

- Qt6 Core/Gui/Widgets/DBus 及所需动态库。
- Qt Wayland 和 XCB platform plugins。
- LayerShellQt library 与 Wayland shell integration plugin。
- KF6GlobalAccel 及其必要运行库。
- X11/XFixes 依赖。
- 图标、desktop、AppStream 和 LICENSE。

构建要求：

- 在足够旧且仍受支持的 glibc 基线上构建，不能直接发布当前 Arch/GCC 16 环境生成的 ELF。
- 不捆绑 glibc、显卡驱动、内核相关库等不应随 AppImage 部署的系统组件。
- 使用 `ldd`、`readelf` 和 AppImage 运行测试确认没有意外依赖构建机路径。
- 在 Ubuntu、Debian、Fedora、openSUSE、Arch 的干净虚拟机验证。
- 提供 SHA-256；后续增加签名、zsync/update information 和 SBOM。

验收门槛：

- 五个发行版家族在未安装 Qt/KF6 开发包时都能启动。
- X11 与 KDE Wayland 所需 Qt plugin 能从 AppImage 内正确加载。
- `--diagnose` 不包含构建机绝对路径。

## 阶段 5：原生 deb、rpm 和 AUR

目标：提供符合各发行版习惯的安装、更新和依赖管理。

### deb

- [ ] 为 Ubuntu LTS 和 Debian stable 确认可用的 Qt6、KF6、LayerShellQt 包名和最低版本。
- [ ] 若 Debian stable 缺少满足要求的 KF6/LayerShellQt，明确选择降级构建，或暂以 AppImage/Flatpak 覆盖，不能偷偷改变功能。
- [ ] 安装 executable、desktop、metainfo、icon、LICENSE 和必要文档。
- [ ] 使用 `dpkg-shlibdeps`/`lintian` 检查依赖和策略问题。
- [ ] 测试安装、升级、降级和卸载。

### rpm

- [ ] 建立 Fedora spec，并按需处理 openSUSE spec/macro 差异。
- [ ] 使用发行版依赖，不在原生 rpm 内私自捆绑 Qt/KF6。
- [ ] 使用 `rpmlint`、动态依赖扫描和干净构建环境验证。
- [ ] Fedora 与 openSUSE 分别执行安装、升级和卸载测试。

### AUR

- [ ] 修正当前缺失的 `git` 构建依赖以及 X11/XFixes 运行依赖。
- [ ] 正式包从固定 release tag/tarball 构建并校验 SHA-256。
- [ ] `package()` 阶段禁止联网。
- [ ] 可先发布源码构建包；只有稳定自动构建来源后再考虑 `gaze-anchor-bin`。
- [ ] 生成并提交 `.SRCINFO`。

原生包的关键原则：**使用目标发行版自身的库构建，不要把 Arch 生成的 ELF 放进 deb 或 rpm。**

验收门槛：

- 包管理器能正确解析全部直接运行依赖。
- 干净系统安装后应用菜单可见并能启动。
- 升级保留配置；卸载默认不删除用户配置。
- 包检查工具无高严重度错误。

## 阶段 6：完成 Flatpak 和 Flathub 准备

目标：形成可复现、最小权限、能力说明准确的沙箱发行版。

任务：

- [ ] 将 app ID、desktop、metainfo、icon 和 Qt desktop file name 统一。
- [ ] 将 `type: dir` 本地 source 改为固定 Git tag/commit 或 release archive，并提供校验值。
- [ ] 更新到受支持的 KDE runtime，不长期固定已过期 runtime。
- [ ] 逐项证明 `finish-args` 的必要性，删除不必要的 host 文件权限。
- [ ] 评估 KGlobalAccel 在沙箱中的实际可用性，优先实现 portal/CLI action。
- [ ] 明确 Flatpak 配置目录和原生配置导入策略。
- [ ] 使用 `flatpak-builder --force-clean` 和 `flatpak run` 做干净构建与运行测试。
- [ ] 先发布 beta remote，稳定后按 Flathub 规范提交。

验收门槛：

- manifest 完全基于可下载、固定、可校验的 source。
- 权限审查通过，应用在拒绝快捷键权限时仍可控制和退出。
- KDE Wayland、fallback X11 和 GNOME Wayland 受限模式均经过验证。

## 阶段 7：自动化正式发布

目标：让正式发行由 tag 驱动，并可重复得到相同来源的产物。

建议流程：

1. 合并 release candidate，确保 CI 全绿。
2. 更新版本、AppStream release 节点、CHANGELOG 和支持矩阵。
3. 创建签名 Git tag，例如 `v0.2.0`。
4. CI 从该 tag 构建 AppImage、deb、rpm 和 source tarball。
5. 在对应容器/虚拟机执行安装和启动 smoke tests。
6. 生成 SHA-256、SBOM 和构建 provenance。
7. 对产物签名并发布到 GitHub Releases。
8. 更新 AUR 和 Flatpak beta；验证后推进稳定渠道。
9. 发布页记录实测发行版、桌面、会话、GPU 和已知问题。
10. 保留构建日志和测试结果，出现回归时可快速撤回渠道版本。

每个 Release 至少包含：

- AppImage。
- Ubuntu/Debian 适配范围明确的 deb。
- Fedora/openSUSE 适配范围明确的 rpm。
- source tarball 与 AUR 构建信息。
- Flatpak 渠道说明。
- SHA-256 校验文件。
- SBOM。
- 支持矩阵、已知问题和配置迁移说明。

## 6. 推荐打包顺序

| 顺序 | 渠道 | 原因 |
|---:|---|---|
| 1 | AUR 源码包 | 当前 Arch/KDE 依赖基础最好，最接近现有开发环境 |
| 2 | AppImage beta | 快速获得多发行版和实机反馈，但必须先完成依赖闭包 |
| 3 | Flatpak beta | KDE runtime 可提供相对统一的 Qt/KF6 环境，适合验证沙箱限制 |
| 4 | deb | 覆盖 Ubuntu、Debian、Mint、Kubuntu/KDE Neon 用户 |
| 5 | rpm | 覆盖 Fedora 和 openSUSE |
| 6 | Flathub stable | 权限、ID、portal 和能力降级稳定后进入正式渠道 |
| 7 | aarch64 | x86_64 流程稳定并获得 ARM64 实机后扩展 |
| 8 | SteamOS Gaming Mode | 独立 Gamescope 实机验证后决定支持等级 |

该顺序与纯跨平台应用常见的“AppImage 优先”略有不同：当前应用已在 Arch/KDE 上有完整依赖和打包骨架，因此先把 AUR 做正确成本最低；AppImage 则必须先解决 Qt/KF6/LayerShellQt 的复杂依赖闭包，不能把现有 AppDir 直接作为正式包发布。

## 7. 建议支持矩阵

| 功能 | KDE Wayland | KDE X11 | 通用 X11 | wlroots Wayland | GNOME Wayland | Gamescope |
|---|---:|---:|---:|---:|---:|---:|
| 设置界面 | 完整 | 完整 | 完整 | 完整 | 完整 | 条件可用 |
| 边缘 Overlay | 完整 | 完整 | 完整 | 实验性 | 不保证 | 实验性 |
| 准星/时钟 | 完整 | 完整 | 完整 | 实验性 | 不保证 | 实验性 |
| 点击穿透 | 需实测确认 | 完整 | 完整 | 实验性 | 不保证 | 实验性 |
| 多显示器 | 完整 | 完整 | 完整 | 待验证 | 设置可用 | 待验证 |
| 全局快捷键 | KGlobalAccel | KGlobalAccel | CLI/待通用后端 | Portal/CLI | Portal/CLI | 条件可用 |
| 托盘 | 完整 | 完整 | 取决于桌面 | 取决于面板 | 通常需扩展 | 不应依赖 |
| 无边框游戏 | 必测 | 必测 | 必测 | 待验证 | 不保证 | 实验性 |
| 独占全屏 | 不承诺 | 不承诺 | 依 WM/游戏 | 不承诺 | 不承诺 | 实验性 |

矩阵中的“完整”只有在对应 Release 实测后才能保留。程序构建成功、设置窗口可见或 desktop 文件能启动，都不能替代 Overlay 行为测试。

## 8. 风险登记

| 风险 | 严重度 | 应对 |
|---|---:|---|
| GNOME Wayland 不提供通用 layer-shell | 极高 | 明确 Limited；能力页解释；建议 X11，不做虚假兼容 |
| 所有 Wayland 会话被强制设为 layer-shell | 极高 | 改为后端能力探测和安全降级 |
| Qt/KF6/LayerShellQt ABI 在发行版间差异大 | 高 | 原生包按发行版构建；AppImage 使用旧基线并收集完整依赖 |
| 可选依赖静默缺失导致同版本功能不同 | 高 | 官方 FULL profile 缺依赖即失败；产物记录 capability manifest |
| AppImage 遗漏 Qt platform/shell plugin | 高 | 自动扫描依赖并在 KDE Wayland、X11 干净 VM 验证 |
| KGlobalAccel 不能覆盖非 KDE 桌面 | 高 | CLI/D-Bus action + portal；支持矩阵明确降级 |
| Flatpak 沙箱限制 D-Bus 和配置访问 | 高 | 最小权限、portal、beta 渠道和沙箱内诊断 |
| 托盘不可用后应用失去控制入口 | 高 | 检测托盘、稳定关闭语义、CLI quit、单实例唤醒 |
| 配置直接 truncate 可能损坏 | 中高 | `QSaveFile`、schema、备份和迁移测试 |
| 当前构建基于滚动发行版新 glibc/libstdc++ | 中高 | AppImage 使用旧构建基线；禁止直接复用 Arch ELF |
| 多屏和 fractional scale 行为不同 | 中高 | 每屏实测、DPR 矩阵、热插拔测试 |
| 应用身份不一致导致商店和配置迁移问题 | 中高 | 阶段 0 一次性统一 ID，并决定是否需要迁移 |
| 缺少测试导致多包格式持续回归 | 中高 | 先建立 CI/CTest，再扩展包数量 |

## 9. 里程碑

| 里程碑 | 可交付结果 |
|---|---|
| M0 | 产品名、App ID、版本和仓库身份统一，clean build 可重复 |
| M1 | 能力探测、受限模式、单实例、无托盘退出、原子配置完成 |
| M2 | 单元测试、安装测试、X11 smoke 和基础 CI 完成 |
| M3 | KDE Wayland、KDE X11、GNOME Wayland 和多屏实机矩阵完成 |
| M4 | 图标、desktop、AppStream、LICENSE 安装完成，AUR 可发布 |
| M5 | AppImage beta 在五个发行版家族启动通过 |
| M6 | Flatpak beta 与沙箱能力测试完成 |
| M7 | deb、rpm 在目标发行版安装/升级/卸载通过 |
| M8 | tag 驱动的签名 Release、校验值、SBOM 和发布文档完成 |
| M9 | Linux 1.0 发布；wlroots、Gamescope 和 ARM64 按实测继续推进 |

不建议直接按自然日估计这些里程碑。先为 M0 至 M2 建 issue 并完成技术验证，再根据实际打包依赖和可获得的实机资源估算后续周期。

## 10. 首批可直接创建的开发任务

建议按以下顺序创建 issue：

1. 确定 GazeAnchor 正式名称、仓库 URL 和 App ID。
2. 统一 CMake target、binary、desktop、Flatpak 和配置 identity。
3. 增加 `--version`、Git commit 和编译 capability 输出。
4. 增加 FULL/X11/MINIMAL 构建 profile，并让 FULL 缺依赖时失败。
5. 修复 Wayland 后端选择，不再仅凭 `XDG_SESSION_TYPE` 强制 layer-shell。
6. 实现后端能力状态及设置界面的降级原因提示。
7. 使用 `QSaveFile`，增加配置 schema、备份和迁移测试。
8. 实现 D-Bus 单实例和 CLI action。
9. 检测托盘能力并确定关闭窗口行为。
10. 添加应用 SVG 图标和真实截图。
11. 添加正式 desktop 与 AppStream metainfo，并纳入 CMake install。
12. 添加 Qt Test/CTest：配置、Profile 和纯几何逻辑。
13. 添加 GitHub Actions clean build、CTest 和 install smoke。
14. 添加 desktop/AppStream validation。
15. 修正 `PKGBUILD`，从固定 tag/tarball 可重复构建。
16. 完成 linuxdeploy AppImage 依赖收集和插件检查。
17. 在五个发行版家族验证 AppImage。
18. 将 Flatpak manifest 改为固定 source，完成 beta 构建。
19. 建立 deb 和安装测试。
20. 建立 Fedora/openSUSE rpm 和安装测试。
21. 建立 KDE Wayland、X11、GNOME Wayland 和 Gamescope 实测记录。
22. 建立 tag 驱动的 Release、SHA-256、签名和 SBOM 流水线。

## 11. 最终发布检查表

### 应用

- [ ] 名称、App ID、可执行文件、配置目录和包名一致。
- [ ] `--version`、`--diagnose` 和 CLI action 可用。
- [ ] 能力受限环境有明确说明，不崩溃、不静默失败。
- [ ] 单实例、托盘缺失和退出行为稳定。
- [ ] 配置原子保存，旧版本升级和损坏恢复通过。

### 构建

- [ ] 所有正式产物来自同一签名 tag。
- [ ] Full build 缺少声明依赖时立即失败。
- [ ] clean build、CTest、install smoke 全部通过。
- [ ] 二进制中不包含构建机绝对 RPATH 或意外依赖。

### 桌面集成

- [ ] 自有图标、desktop、AppStream、LICENSE 均由 CMake 安装。
- [ ] desktop 和 AppStream 校验通过。
- [ ] 应用菜单名称、图标和 Flatpak identity 正确。
- [ ] 发布页有真实截图和准确描述。

### 包

- [ ] AUR 可从固定 source 重复构建。
- [ ] AppImage 在 Ubuntu、Debian、Fedora、openSUSE、Arch 干净环境启动。
- [ ] deb 在目标 Ubuntu/Debian/Mint 环境安装、升级和卸载通过。
- [ ] rpm 在 Fedora/openSUSE 安装、升级和卸载通过。
- [ ] Flatpak 使用固定 source、最小权限并通过沙箱测试。

### 功能验证

- [ ] KDE Wayland 与 KDE X11 的 Tier 1 测试通过。
- [ ] 通用 X11 支持范围经实测确认。
- [ ] GNOME Wayland 正确进入受限模式。
- [ ] 多屏、混合 DPI、热插拔和 GPU 矩阵有记录。
- [ ] 原生游戏、Proton、无边框和 Gamescope 的结果如实发布。
- [ ] Overlay 不抢焦点；支持环境中点击完全穿透。

### 发布

- [ ] 提供 SHA-256、签名、SBOM 和 provenance。
- [ ] 提供 CHANGELOG、升级说明、支持矩阵和已知问题。
- [ ] 发布渠道中的版本、元数据和下载链接一致。
- [ ] 有回滚或下架有问题版本的流程。

## 12. 最终建议

最现实的路线是：

> 保留当前 C++20 + Qt 6 Widgets 架构，以 KDE Wayland 和 KDE/X11 为首发核心；先完成应用身份、能力探测、配置可靠性、桌面集成和 CI，再依次交付 AUR、真正便携的 AppImage、Flatpak beta、deb 和 rpm；通过 tag、安装测试、实机矩阵、签名和 SBOM 把一次性构建升级为可持续发行流程。

第一优先级不是立刻增加五种包，而是完成 **M0 至 M2**。如果产品 identity、构建能力和测试基线尚未稳定，越早复制出 deb、rpm、Flatpak 和 AppImage，后续需要同步修复的发行分支就越多。

Linux 1.0 的合理承诺应是：

- 主流发行版有至少一种经过验证的安装渠道。
- KDE Plasma Wayland 和 X11 提供可靠的核心 Overlay 体验。
- 通用 X11 按已验证能力提供支持。
- GNOME Wayland 明确受限，不作无法兑现的 Overlay 承诺。
- wlroots、SteamOS Gaming Mode 和 Gamescope 在实机通过前保持 Experimental。
- 每个发布产物都来自同一 tag，可校验、可追溯、可复现其构建过程。
