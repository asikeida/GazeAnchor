# MotionStabilizer 主流 Linux 发行版覆盖规划

> 文档状态：初版规划  
> 编写日期：2026-09-03  
> 适用仓库：当前 C# 12 / .NET 8 / WPF 版本  
> 目标：让 MotionStabilizer 能在主流 Linux 发行版上安装、启动，并在明确支持的桌面会话中可靠提供视觉叠加功能。

## 1. 执行摘要

当前项目不是 Linux 项目，而是明确绑定 Windows 的桌面应用：

- 主项目目标框架为 `net8.0-windows`。
- UI 使用 WPF 和少量 WinForms。
- 窗口置顶、点击穿透、全局快捷键、显示器枚举、Raw Input、键盘状态、单实例均依赖 Win32 API。
- 动态圆点渲染依赖 Direct3D 11、DXGI、Direct2D 和 DirectComposition。
- 手柄输入依赖 XInput。
- 当前 CI 仅在 `windows-latest` 上执行。

因此，覆盖主流 Linux 发行版不能通过增加几个打包脚本完成。正确路线是：

1. 保留现有 Windows 应用，避免破坏当前可用版本。
2. 提取平台无关的配置、几何、动画和业务逻辑。
3. 新建 Linux 设置应用和 Linux 平台服务。
4. 分别实现 X11、支持 layer-shell 的 Wayland，以及受限桌面环境的后端。
5. 使用 AppImage、deb、rpm、AUR 和 Flatpak 覆盖发行渠道。
6. 使用 CI、嵌套合成器和真实系统建立可重复验证矩阵。

必须提前接受一个产品事实：**发行版覆盖不等于桌面会话功能完全一致。** Ubuntu、Fedora、Debian 等发行版可以安装同一应用，但 KDE Wayland、GNOME Wayland、wlroots 和 X11 对 Overlay 的能力限制完全不同。

推荐的首发支持承诺：

| 支持等级 | 环境 | 承诺 |
|---|---|---|
| Tier 1 | 主流发行版的 X11 会话 | 完整功能，包括静态 Overlay、动态圆点、鼠标/键盘/手柄输入、全局快捷键、多屏 |
| Tier 1 | KDE Plasma Wayland | 静态 Overlay、准星、时钟、OSD、多屏、快捷键；动态鼠标/键盘输入按协议能力降级 |
| Tier 2 | Sway、Hyprland、Wayfire 等支持 layer-shell 的 Wayland | 静态 Overlay 和多屏正式支持；快捷键、托盘和动态输入按运行时能力提供 |
| Tier 3 | GNOME Wayland | 应用与配置可运行；普通客户端无法保证全屏 Overlay，首发明确标记受限 |
| Experimental | SteamOS / Gamescope Gaming Mode | 独立实验目标，实机通过后再提升支持等级 |

## 2. 当前项目基线

### 2.1 当前技术栈

| 项目 | 当前实现 | Linux 影响 |
|---|---|---|
| 语言 | C# 12 | 可继续使用，语言不是障碍 |
| 运行时 | .NET 8 | Linux 可用，但项目已锁定 Windows；正式 Linux 版建议升级 .NET 10 LTS |
| UI | WPF | Windows 专属，Linux 必须迁移或新建 UI |
| 静态 Overlay | WPF 透明窗口 | Linux 必须重写窗口后端 |
| 动态圆点 | Vortice + DirectComposition | DirectX 专属，Linux 渲染后端必须重写 |
| 窗口控制 | user32.dll | Linux 必须按 X11/Wayland 分别实现 |
| 鼠标输入 | Win32 Raw Input | X11 可实现；Wayland 普通客户端不可等价实现 |
| 键盘输入 | GetAsyncKeyState | X11 可实现；Wayland 普通客户端不可等价实现 |
| 手柄 | XInput | Linux 建议迁移 SDL3 Gamepad API |
| 全局快捷键 | RegisterHotKey | X11 使用 XGrabKey；Wayland 使用 portal 或桌面快捷键调用 CLI |
| 托盘 | WinForms NotifyIcon | Linux 使用 StatusNotifierItem/AppIndicator，并支持无托盘模式 |
| 单实例 | Windows Mutex + 窗口消息 | Linux 使用 D-Bus name 或 Unix socket |

关键文件证据：

- `MotionStabilizer/MotionStabilizer.csproj` 使用 `net8.0-windows`、WPF、WinForms 和 Vortice DirectX 包。
- `MotionStabilizer/Services/Win32Interop.cs` 集中实现 Windows 窗口、快捷键、显示器和输入 API。
- `MotionStabilizer/Overlay/DirectCompositionMotionRenderer.cs` 直接依赖 DirectComposition、Direct2D、Direct3D 11 和 DXGI。
- `MotionStabilizer.Tests/MotionStabilizer.Tests.csproj` 同样锁定 `net8.0-windows`。
- `.github/workflows/dotnet.yml` 当前只运行 Windows CI。

### 2.2 可复用资产

以下内容应提取后继续复用，不应重写：

- 配置结构、枚举和默认值。
- Profile 保存、加载、删除规则。
- `ObservableObject` 和配置变更通知逻辑。
- 快捷键绑定的领域模型和显示字符串逻辑。
- OSD 文案映射。
- 安全区域、宽高比、分屏和位置限制算法。
- 动态圆点的区域计算、速度合成、视差、衰减、循环和呼吸动画算法。
- 手柄摇杆归一化、死区和速度计算。
- 已有几何、配置、快捷键、显示器和手柄数学测试。
- 中英文文案内容和现有截图素材。

### 2.3 必须替换的资产

以下内容无法直接移植到 Linux：

- WPF `Window`、Page、ResourceDictionary 和 WPF Shape。
- WinForms `NotifyIcon` 和系统颜色对话框。
- `System.Windows.Media.Color` 等配置层 WPF 类型。
- `user32.dll`、`gdi32.dll`、Raw Input、RegisterHotKey、XInput。
- DirectComposition、DXGI、Direct2D 和 Direct3D 11 渲染资源。
- Windows manifest、ICO 专属资源和 PerMonitorV2 声明。
- 依赖窗口消息的单实例和唤醒机制。

## 3. 覆盖范围定义

### 3.1 首批发行版

| 发行版家族 | 具体测试目标 | 发布产物 |
|---|---|---|
| Ubuntu | 最新两个仍受支持的 LTS | AppImage、deb、Flatpak |
| Debian | 当前 stable | AppImage、deb、Flatpak |
| Fedora | 当前版和前一版 | AppImage、rpm、Flatpak |
| openSUSE | Tumbleweed 和当前 Leap | AppImage、rpm、Flatpak |
| Arch Linux | rolling | AppImage、AUR、Flatpak |
| Linux Mint | 当前主版本 | 继承 Ubuntu deb，并单独冒烟测试 |
| KDE Neon | 当前 User Edition | 继承 Ubuntu deb，并做 KDE Wayland专项测试 |
| SteamOS | 当前稳定版 Desktop Mode | AppImage；Gaming Mode 单独标记 Experimental |

首发 CPU 架构为 `linux-x64`。`linux-arm64` 在 x64 稳定后进入第二阶段。Alpine/musl、32 位 x86 和非桌面服务器发行版不属于首批范围。

### 3.2 显示会话

必须检测真实能力，不能只根据发行版名称判断：

| 会话 | 识别方式 | 后端 |
|---|---|---|
| X11 | `XDG_SESSION_TYPE=x11` 且可连接 DISPLAY | X11/XCB/Xlib 后端 |
| KDE Wayland | Wayland + KDE 桌面 + layer-shell 可用 | Wayland layer-shell 后端 |
| wlroots Wayland | Wayland + layer-shell registry 可用 | Wayland layer-shell 后端 |
| GNOME Wayland | Wayland + GNOME/Mutter，通常无 layer-shell | 受限模式 |
| Gamescope | Gamescope 会话或指定 Wayland socket | Experimental Gamescope 路径 |

### 3.3 “覆盖”的完成定义

一个发行版只有同时满足以下要求才算被覆盖：

- 用户不安装 .NET SDK 也能运行。
- 安装包包含所有允许捆绑的运行时依赖。
- 设置窗口能启动、保存配置并正常退出。
- 应用能识别当前显示会话及可用能力。
- 支持的功能有自动化或实机验证记录。
- 不支持的功能在 UI 中禁用并解释原因，不静默失败。
- 安装、升级和卸载不破坏用户配置。
- README 和发布页准确标注该发行版及桌面会话的支持等级。

## 4. 推荐目标架构

### 4.1 项目结构

建议逐步重构为：

```text
MotionStabilizer.slnx
src/
  MotionStabilizer.Core/
  MotionStabilizer.Rendering.Core/
  MotionStabilizer.Platform.Abstractions/
  MotionStabilizer.Windows/
  MotionStabilizer.Linux/
  MotionStabilizer.Linux.X11/
  MotionStabilizer.Linux.Wayland/
tests/
  MotionStabilizer.Core.Tests/
  MotionStabilizer.Windows.Tests/
  MotionStabilizer.Linux.Tests/
packaging/
  appimage/
  deb/
  rpm/
  aur/
  flatpak/
  steam/
```

职责边界：

| 模块 | 职责 |
|---|---|
| Core | 配置、Profile、领域模型、多语言键、数据迁移 |
| Rendering.Core | 几何、绘制命令、MotionEngine、动画状态 |
| Platform.Abstractions | 显示器、Overlay、输入、快捷键、托盘、单实例接口 |
| Windows | 保留 WPF、Vortice 和 Win32 实现 |
| Linux | Avalonia 设置 UI、生命周期、能力诊断 |
| Linux.X11 | X11 Overlay、输入、快捷键、显示器 |
| Linux.Wayland | layer-shell surface、output、portal、能力探测 |

### 4.2 平台接口

最低限度定义以下接口，避免在业务代码中散布 `OperatingSystem.IsLinux()`：

```csharp
public interface IOverlaySurface;
public interface IOverlayRenderer;
public interface IDisplayTopology;
public interface IGlobalShortcutService;
public interface IPointerMotionSource;
public interface IKeyboardMotionSource;
public interface IGamepadSource;
public interface ITrayService;
public interface ISingleInstanceService;
public interface IPlatformCapabilities;
```

接口返回值必须表达 `Supported`、`Unavailable`、`PermissionRequired`、`Degraded` 和失败原因，不能只返回布尔值。

### 4.3 UI 路线

Linux 设置 UI 推荐使用 Avalonia：

- 保持 C#/.NET 技术栈。
- XAML 思维与 WPF 接近。
- Linux 桌面成熟度高于 .NET MAUI。
- 可以使用 Skia 渲染设置界面和预览。

短期不迁移 Windows WPF UI。先让 Windows 项目引用共享 Core，Linux 建立独立 Avalonia UI。等 Linux 稳定后再决定是否统一 Windows UI。

不推荐 .NET MAUI、Electron 或仅使用 XWayland：这些方案都不能解决 Wayland layer-shell、后台输入和全局快捷键限制。

### 4.4 Linux Overlay 路线

#### X11

目标能力：

- ARGB 透明窗口。
- `_NET_WM_STATE_ABOVE` 或合适的 override-redirect 策略。
- XFixes 空 input region 实现点击穿透。
- XI2 RawMotion 读取后台鼠标相对运动。
- XGrabKey 实现全局快捷键。
- RandR 枚举显示器、旋转、位置和热插拔。
- EWMH 获取前台窗口及窗口模式所需几何信息。

X11 是 Linux 首发实现完整 Windows 功能对等的主要目标。

#### KDE 与 wlroots Wayland

目标能力：

- 使用 `wlr-layer-shell-unstable-v1` 创建 overlay layer surface。
- `exclusive_zone=0`，不挤占桌面空间。
- `keyboard_interactivity=none`，不抢键盘焦点。
- 设置空 input region，鼠标点击穿透。
- 每个 `wl_output` 创建独立 surface。
- 支持 output 热插拔、fractional-scale 和 viewporter。
- 使用 EGL/OpenGL 渲染，并保留 shm 软件回退。

Wayland 普通客户端无法像 Win32 Raw Input 一样被动读取全局鼠标和 WASD，因此动态圆点必须按输入源降级：

- 静态边缘、准星、时钟：正式支持。
- 手柄驱动动态圆点：可支持，使用 SDL3。
- 鼠标和 WASD 驱动动态圆点：默认不可用，除非存在明确、经用户授权的协议能力。

#### GNOME Wayland

GNOME Mutter 通常不提供通用 layer-shell。首发策略：

- 设置 UI、配置和 Profile 正常运行。
- 能力页明确显示“当前桌面不提供全屏 Overlay 协议”。
- 提示用户选择 X11 会话以获得完整功能。
- 不使用普通置顶窗口假装已经支持全屏 Overlay。
- 后续可评估独立 GNOME Shell 扩展，但不把扩展作为首发阻塞项。

#### Gamescope

Gamescope 必须作为单独目标验证：

- 区分运行在宿主桌面和连接 Gamescope 内部 socket 两种模式。
- Steam Deck Desktop Mode 可按普通 KDE Wayland 验证。
- Gaming Mode 只有真实设备测试通过后才能宣传支持。
- 不采用 DLL/SO 注入或私有 Steam Overlay 接口。

### 4.5 渲染路线

建议分两步：

1. 静态 Overlay 和低频预览先使用 Skia/软件缓冲验证正确性。
2. 动态圆点使用 EGL/OpenGL 后端，目标支持 30 至 360 Hz，并保留软件回退。

必须从当前 `DirectCompositionMotionRenderer` 中提取：

- Motion zone 数据结构。
- 圆点初始化和随机种子策略。
- 鼠标、键盘、手柄速度合成。
- wrap、视差缩放、alpha fade 和 breathing 动画。

不得把 DirectComposition 的交换链、窗口、电源策略和 Windows 定时器迁入共享层。

### 4.6 输入和快捷键

| 能力 | X11 | Wayland |
|---|---|---|
| 鼠标相对运动 | XI2 RawMotion | 普通客户端不可用；按授权协议能力降级 |
| WASD 状态 | XI2/XQueryKeymap | 普通客户端不可用 |
| 手柄 | SDL3 | SDL3，受设备和沙箱权限影响 |
| 全局快捷键 | XGrabKey | xdg-desktop-portal GlobalShortcuts |
| 快捷键备用方案 | CLI/D-Bus action | CLI/D-Bus action |

建议提供稳定 CLI action：

```text
gaze-anchor action toggle-overlay
gaze-anchor action toggle-crosshair
gaze-anchor action toggle-clock
gaze-anchor action toggle-motion-dots
gaze-anchor action cycle-shape
```

当 portal 不可用时，用户可以在 KDE、GNOME、Sway 或 Hyprland 的系统快捷键设置中调用这些命令。

### 4.7 配置与数据目录

Linux 使用 XDG Base Directory：

```text
$XDG_CONFIG_HOME/gaze-anchor/
$XDG_STATE_HOME/gaze-anchor/
$XDG_CACHE_HOME/gaze-anchor/
```

需要补充：

- 配置 schema version。
- Windows 旧配置兼容或一次性导入。
- 原子写入：临时文件、flush、rename。
- 损坏配置备份和用户提示。
- 显示器 ID 使用平台命名空间，避免 Windows ID 与 Linux connector/EDID 混淆。
- Flatpak 与原生包的配置导入说明。

产品名称是否正式改为 GazeAnchor 应单独确认；在确认前，代码迁移可以继续使用 MotionStabilizer namespace，避免一次性大范围重命名干扰功能移植。

## 5. 分阶段实施计划

### 阶段 0：建立基线和可行性门

目标：先证明各显示协议能完成核心 Overlay，再投入 UI 迁移。

任务清单：

- [ ] 为当前 Windows 版本记录构建、测试和基础运行基线。
- [ ] 建立 X11 透明、置顶、点击穿透原型。
- [ ] 建立 KDE Wayland layer-shell 原型。
- [ ] 建立 Sway 或 Hyprland layer-shell 原型。
- [ ] 在 GNOME Wayland 验证预期受限行为。
- [ ] 在 Gamescope 嵌套环境验证 surface 可见性。
- [ ] 验证 Intel、AMD、NVIDIA 至少各一套图形环境。
- [ ] 记录多屏、fractional scale、全屏无边框、VRR/HDR 已知表现。

验收门槛：

- X11、KDE Wayland、至少一个 wlroots 合成器能显示测试 Overlay。
- Overlay 不抢焦点且点击穿透。
- 屏幕热插拔后 surface 能正确重建。
- GNOME Wayland 明确进入受限模式，而不是静默失败。
- 形成 Go/No-Go 记录，确认 Linux Wayland native backend 的具体实现方式。

### 阶段 1：提取跨平台 Core

目标：让纯业务逻辑在 Linux CI 上编译和测试，同时保持 Windows 行为不变。

任务清单：

- [ ] 新建 solution 和 `MotionStabilizer.Core`。
- [ ] 新建 `MotionStabilizer.Rendering.Core`。
- [ ] 新建 `MotionStabilizer.Platform.Abstractions`。
- [ ] 引入平台无关 `RectI`、`RectF`、`Rgba32`、`DisplayInfo`。
- [ ] 从配置模型移除 WPF Color、Brush、FontFamily。
- [ ] 提取安全区域、分屏、尺寸和坐标限制算法。
- [ ] 提取 Motion zone 与 MotionEngine。
- [ ] 提取手柄摇杆数学，不携带 XInput DLL 调用。
- [ ] 将纯逻辑测试迁移到 `MotionStabilizer.Core.Tests`。
- [ ] 保持当前 JSON 字段和 Windows配置兼容。
- [ ] Windows WPF 项目改为引用共享 Core。

验收门槛：

- Core 在 Windows 和 Ubuntu runner 上构建通过。
- Core 测试在 Windows 和 Ubuntu 上全部通过。
- Core 不引用 `System.Windows.*`、WinForms、Vortice、user32 或 XInput。
- 当前 Windows 发布版本主要行为无回归。

### 阶段 2：Linux 设置应用

目标：完成不依赖 Overlay 后端也能运行的 Linux Avalonia 设置应用。

任务清单：

- [ ] 新建 Avalonia Linux executable。
- [ ] 迁移主窗口、边缘叠加、动态圆点、准星、时钟、快捷键和全局选项页面。
- [ ] 将 WPF code-behind 转为 ViewModel 和 command。
- [ ] 迁移中英文资源。
- [ ] 使用 SVG/PNG 跨平台图标。
- [ ] 实现 XDG 配置目录和原子保存。
- [ ] 实现 D-Bus name 或 Unix socket 单实例。
- [ ] 增加“系统能力”诊断页面。
- [ ] 无托盘环境下确保窗口关闭可以退出。

能力页至少显示：

- 当前发行版和桌面环境。
- X11/Wayland/Gamescope 会话类型。
- Overlay 协议可用性。
- 全局快捷键 portal 可用性。
- Raw pointer、keyboard、gamepad 可用性。
- 托盘可用性。
- 当前功能降级原因。

验收门槛：

- Ubuntu、Fedora、Arch 上设置窗口可启动。
- 中英文切换、配置保存和 Profile 管理正常。
- 第二实例会激活第一实例。
- GNOME 没有托盘扩展时应用仍有明确退出路径。

### 阶段 3：X11 完整后端

目标：在 X11 上达到当前 Windows 版本的主要功能对等。

任务清单：

- [ ] 实现显示器枚举、位置、旋转、DPI 和热插拔。
- [ ] 实现 ARGB 透明 Overlay 窗口。
- [ ] 实现置顶、不抢焦点、任务栏隐藏和点击穿透。
- [ ] 实现静态边缘、准星、时钟和 OSD。
- [ ] 实现动态圆点 EGL/OpenGL 渲染。
- [ ] 实现 XI2 RawMotion。
- [ ] 实现 WASD 状态读取。
- [ ] 实现 XGrabKey 全局快捷键。
- [ ] 集成 SDL3 手柄和热插拔。
- [ ] 实现 EWMH 前台窗口与 Window 模式几何。
- [ ] 增加软件渲染回退。

验收门槛：

- Ubuntu X11、Debian X11、Fedora X11、Arch X11 至少各一轮通过。
- 窗口化和无边框全屏游戏 Overlay 可见。
- 鼠标点击、滚轮和拖动全部穿透。
- 游戏保持焦点，Overlay 不出现在任务栏。
- 鼠标、WASD、手柄驱动动态圆点正常。
- 多显示器负坐标、旋转和混合 DPI 正常。
- 30/60/120/144/165/240 Hz 无明显抖动。
- 连续运行两小时无持续内存或 GPU 资源增长。

### 阶段 4：Wayland 静态 Overlay

目标：正式支持 KDE Wayland 和主流 wlroots 合成器的静态视觉锚点。

任务清单：

- [ ] 实现 Wayland registry 和 output 生命周期。
- [ ] 实现 layer-shell surface。
- [ ] 实现每输出独立 Overlay。
- [ ] 实现空 input region 和 keyboard interactivity none。
- [ ] 实现 EGL/OpenGL 与 shm fallback。
- [ ] 实现 output 热插拔和缩放更新。
- [ ] 支持 fractional-scale 和 viewporter 时正确适配。
- [ ] UI 根据能力禁用 Window 模式、Raw Mouse 和 WASD。
- [ ] 在不支持 layer-shell 的环境中展示明确错误。

验收门槛：

- KDE Plasma Wayland 通过。
- Sway 和 Hyprland 至少通过一个稳定版和一个滚动版测试环境。
- 边缘、准星、时钟和 OSD 不抢焦点并点击穿透。
- 全屏无边框 surface 上方显示行为符合公开说明。
- 不可用功能不会继续显示为“已启用”。

### 阶段 5：Wayland 快捷键、托盘和手柄

目标：完善非 Raw Input 能力及降级路径。

任务清单：

- [ ] 接入 `xdg-desktop-portal` GlobalShortcuts。
- [ ] 实现 portal 权限授权、拒绝和会话恢复。
- [ ] 实现 CLI/D-Bus action 备用方案。
- [ ] 实现 StatusNotifierItem/AppIndicator。
- [ ] 支持无托盘模式。
- [ ] 集成 SDL3 手柄和设备热插拔。
- [ ] 评估 InputCapture portal，但不把它宣传为被动 Raw Input 等价替代。

验收门槛：

- Portal 可用时能授权、注册和恢复快捷键。
- Portal 不可用时用户能绑定 CLI action。
- 快捷键冲突、授权拒绝和 portal 崩溃都有明确反馈。
- 手柄拔插不会导致崩溃或输入源锁死。
- 托盘不可用时程序不会成为无法控制的后台进程。

### 阶段 6：打包与安装

目标：形成无需 SDK、可安装、可升级的发行产物。

#### AppImage

- [ ] 使用 self-contained .NET 发布。
- [ ] 选择足够旧且受支持的 glibc 构建基线。
- [ ] 捆绑 Avalonia/Skia、SDL 和项目 native libraries。
- [ ] 使用 linuxdeploy 或等效工具收集运行库。
- [ ] 验证没有意外依赖开发机的 .NET、Qt、GTK 开发包。
- [ ] 文档说明无 FUSE 环境的运行方式。

#### deb

- [ ] 提供 Debian policy 合理的目录布局。
- [ ] 安装 executable、desktop、AppStream metainfo、图标和许可证。
- [ ] 使用 self-contained .NET，避免要求用户配置 Microsoft 软件源。
- [ ] 在 Ubuntu LTS、Debian stable、Linux Mint 上测试安装和卸载。

#### rpm

- [ ] 建立 Fedora spec。
- [ ] 建立或验证 openSUSE spec 差异。
- [ ] 在 Fedora 和 openSUSE 干净环境验证动态依赖。
- [ ] 安装 desktop、AppStream、图标和许可证。

#### AUR

- [ ] 提供源码构建包。
- [ ] 可选提供 `-bin` 包。
- [ ] 固定 tag、source 和 SHA-256。
- [ ] 禁止在 `package()` 阶段联网。

#### Flatpak

- [ ] 使用 Wayland 和 fallback X11 socket。
- [ ] 使用 portal，不直接申请不必要的 host 文件权限。
- [ ] 谨慎处理手柄设备权限。
- [ ] 在 capability 页面解释沙箱限制。
- [ ] 先发布 beta remote，稳定后提交 Flathub。

验收门槛：

- 干净虚拟机无 .NET SDK 时可安装并启动。
- desktop 文件和 AppStream metadata 校验通过。
- 包内无缺失动态库。
- 升级保留用户配置。
- 卸载不删除用户配置，除非用户明确选择清理。

### 阶段 7：CI、自动测试和实机矩阵

#### 每次提交 CI

| 作业 | 环境 | 内容 |
|---|---|---|
| Core | windows-latest | build + unit tests |
| Core | ubuntu-latest | build + unit tests |
| Windows | windows-latest | WPF build + Windows tests |
| Linux | ubuntu-latest | Avalonia/Linux build + Linux unit tests |
| X11 smoke | Ubuntu container/VM + Xvfb | 窗口属性、input region、快捷键基础测试 |
| Wayland smoke | Weston headless | registry、surface、output 生命周期 |
| layer-shell smoke | nested Sway | layer-shell、输入穿透、多输出模拟 |

#### 发布候选实机测试

| 发行版 | 桌面 | 会话 | 优先级 |
|---|---|---|---|
| Ubuntu LTS | GNOME | Wayland | 必测受限模式 |
| Ubuntu LTS | GNOME | X11 | 必测完整模式 |
| KDE Neon/Kubuntu | Plasma | Wayland | 必测 |
| Debian stable | GNOME 或 KDE | X11 | 必测 |
| Fedora | GNOME | Wayland | 必测受限模式 |
| Fedora KDE | Plasma | Wayland | 必测 |
| Arch | Plasma | Wayland | 必测 |
| Arch | Hyprland/Sway | Wayland | 必测 |
| openSUSE Tumbleweed | Plasma | Wayland | 必测 |
| Linux Mint | Cinnamon | X11 | 必测 |
| SteamOS | Desktop Mode | Wayland | 必测 |
| SteamOS | Gaming Mode | Gamescope | Experimental |

#### 新增测试类别

- [ ] 配置版本迁移和损坏恢复。
- [ ] 原子保存中断测试。
- [ ] 跨平台颜色和字体映射。
- [ ] Renderer golden image。
- [ ] X11 window property 和 input region。
- [ ] X11 快捷键冲突与注销。
- [ ] Wayland protocol 生命周期。
- [ ] output 热插拔和 fractional scaling。
- [ ] SDL3 手柄热插拔。
- [ ] AppImage/deb/rpm 安装、启动和卸载。
- [ ] Flatpak 沙箱能力检测。

### 阶段 8：发布和支持

目标：形成可维护的 Linux 正式版，而不是一次性构建。

每个 Release 必须提供：

- AppImage、deb、rpm 和 AUR source 信息。
- Flatpak beta 或稳定渠道信息。
- SHA-256 校验值。
- SBOM。
- 构建 provenance 或签名。
- 支持矩阵和已验证版本。
- X11、KDE Wayland、wlroots、GNOME Wayland、Gamescope 的能力说明。
- 已知问题和回退方式。
- 从上一版本升级的配置迁移说明。

正式版发布门槛：

- 所有 Tier 1 环境通过实机测试。
- Tier 2 至少覆盖两个 compositor。
- GNOME Wayland 正确显示受限状态。
- 安装包在无 SDK 干净系统启动成功。
- 高优先级崩溃、配置损坏、焦点抢占和输入阻塞问题为零。

## 6. 打包策略优先级

推荐顺序：

| 顺序 | 渠道 | 原因 |
|---:|---|---|
| 1 | AppImage | 最快覆盖多个发行版，保留较完整系统能力，适合早期测试 |
| 2 | deb | 覆盖 Ubuntu、Debian、Mint、KDE Neon 大量用户 |
| 3 | rpm | 覆盖 Fedora 和 openSUSE |
| 4 | AUR | 当前目标用户和滚动发行版测试反馈快 |
| 5 | Flatpak beta | 验证 sandbox、portal、Wayland 和 Flathub 规则 |
| 6 | Flathub | 在权限与能力降级稳定后进入正式渠道 |
| 7 | linux-arm64 | x64 稳定后扩展 |
| 8 | SteamOS Gaming Mode | 独立实机和 Gamescope 验证后发布 |

AppImage 解决“能运行”，deb/rpm/AUR 解决“原生安装体验”，Flatpak 解决“统一商店分发和更新”。不能只做一种格式就宣称覆盖所有 Linux 用户。

## 7. 产品支持矩阵

| 功能 | X11 | KDE Wayland | wlroots Wayland | GNOME Wayland | Gamescope |
|---|---:|---:|---:|---:|---:|
| 设置界面 | 完整 | 完整 | 完整 | 完整 | 条件可用 |
| 静态边缘 Overlay | 完整 | 完整 | 完整 | 不保证 | 实验性 |
| 准星 | 完整 | 完整 | 完整 | 不保证 | 实验性 |
| 时钟/OSD | 完整 | 完整 | 完整 | 不保证 | 实验性 |
| 点击穿透 | 完整 | 完整 | 完整 | 无可靠 Overlay | 实验性 |
| 多显示器 | 完整 | 完整 | 完整 | UI 可用 | 通常单虚拟输出 |
| Window 模式跟随游戏窗口 | 完整 | 不可通用实现 | 不可通用实现 | 不可通用实现 | 不可通用实现 |
| 全局快捷键 | 完整 | Portal/CLI | Portal/CLI | Portal/CLI | 条件可用 |
| Raw Mouse 动态圆点 | 完整 | 默认不可用 | 默认不可用 | 不可用 | 不可用 |
| WASD 动态圆点 | 完整 | 默认不可用 | 默认不可用 | 不可用 | 不可用 |
| 手柄动态圆点 | 完整 | 可用 | 可用 | UI 内可用，Overlay 受限 | 实验性 |
| 托盘 | 完整 | 完整 | 取决于面板 | 通常需要扩展 | 通常不可依赖 |

## 8. 风险登记

| 风险 | 严重度 | 应对措施 |
|---|---:|---|
| GNOME Wayland 无通用 layer-shell | 极高 | 明确 Tier 3；提示 X11；后续评估 Shell 扩展 |
| Wayland 禁止后台 Raw Mouse/WASD | 极高 | 功能降级；优先手柄；不绕过安全模型 |
| Gamescope 会话拓扑复杂 | 高 | 独立实验目标和启动路径；Steam Deck 实机验证 |
| Avalonia 普通窗口不能承担 layer-shell | 高 | 设置 UI 与 Overlay native surface 分离 |
| fractional scale 和多屏差异 | 高 | 每输出独立 surface；协议能力探测 |
| NVIDIA EGL/透明 surface 差异 | 高 | Intel/AMD/NVIDIA 实机测试；软件回退 |
| Flatpak 手柄和输入权限 | 高 | 最小权限；capability probe；不默认申请全部设备 |
| 360 Hz 调度和功耗 | 高 | presentation timing/frame callback；限制无意义空刷新 |
| 配置模型包含 WPF 类型 | 中高 | Core 中引入平台中立颜色和字体模型 |
| Windows 功能回归 | 中高 | 保留 WPF executable；先提取 Core；Windows CI 持续运行 |
| 双语言/原生 interop 维护成本 | 中 | 将原生层限制在窗口、显示协议和输入边界 |
| 包在新发行版因 glibc 失败 | 中高 | 旧基线构建；干净 VM 和依赖扫描 |

## 9. 里程碑建议

不按未经估算的自然日承诺进度，使用可验证里程碑推进：

| 里程碑 | 结果 |
|---|---|
| M0 | X11、KDE Wayland、wlroots 技术原型完成，GNOME/Gamescope 边界确认 |
| M1 | Core 提取完成，Windows 和 Linux runner 均通过纯逻辑测试 |
| M2 | Linux Avalonia 设置应用可运行，配置、语言、Profile、单实例完成 |
| M3 | X11 完整功能达到 Beta，包括动态圆点和输入 |
| M4 | KDE/wlroots Wayland 静态 Overlay 达到 Beta |
| M5 | Portal 快捷键、托盘、SDL3 手柄和降级 UI 完成 |
| M6 | AppImage、deb、rpm、AUR 进入 Release Candidate |
| M7 | Flatpak beta 和主流发行版实机矩阵完成 |
| M8 | Linux 1.0 发布；SteamOS/Gamescope 继续 Experimental |

## 10. 首批可直接创建的开发任务

建议按以下顺序创建 issue：

1. 建立 solution、Directory.Build.props 和锁定 SDK。
2. 将项目升级规划从 .NET 8 转向 .NET 10 LTS。
3. 提取平台中立颜色、矩形和显示器模型。
4. 提取 Core 配置和 Profile 服务。
5. 提取几何与 MotionEngine。
6. 将纯逻辑测试迁移为跨平台测试项目。
7. 建立 Ubuntu Core CI。
8. 开发 X11 Overlay spike。
9. 开发 Wayland layer-shell spike。
10. 记录 GNOME Wayland 和 Gamescope capability spike 结果。
11. 新建 Avalonia Linux shell。
12. 定义 Platform.Abstractions 接口和 capability 状态模型。
13. 实现 XDG 配置、日志和单实例。
14. 实现 X11 正式后端。
15. 实现 Wayland 正式后端。
16. 集成 SDL3。
17. 接入 GlobalShortcuts portal 和 CLI action。
18. 建立 AppImage staging 和依赖扫描。
19. 建立 deb/rpm/AUR。
20. 建立 Flatpak beta 和安装冒烟测试。

## 11. 最终验收清单

### 构建

- [ ] Windows 和 Linux clean build 均成功。
- [ ] Core 在 Linux 上没有 Windows-only 引用。
- [ ] 所有 release artifact 来自同一 tag。
- [ ] `linux-x64` self-contained 产物不要求用户安装 .NET。

### 功能

- [ ] X11 达到静态 Overlay、动态圆点、输入和快捷键完整功能。
- [ ] KDE Wayland 和 wlroots 达到静态 Overlay 正式支持。
- [ ] GNOME Wayland 受限模式准确可见。
- [ ] 多屏、DPI、旋转、热插拔通过。
- [ ] Overlay 不抢焦点且点击穿透。

### 打包

- [ ] AppImage 在五个发行版家族的干净环境启动。
- [ ] deb 在 Ubuntu/Debian/Mint 安装、升级、卸载通过。
- [ ] rpm 在 Fedora/openSUSE 安装、升级、卸载通过。
- [ ] AUR 可从 tag 重复构建。
- [ ] Flatpak 权限最小化且 capability 页面准确。

### 质量

- [ ] Core 单元测试跨平台通过。
- [ ] X11 和 Wayland smoke tests 通过。
- [ ] Renderer golden image 无意外变化。
- [ ] 两小时稳定性测试无持续资源增长。
- [ ] 配置损坏和写入中断可恢复。

### 文档

- [ ] README 不再将“发行版可安装”混同于“全部桌面功能完整”。
- [ ] 每种会话列出支持、降级和不支持项。
- [ ] 发布页列出经过验证的发行版、桌面、显示协议和 GPU。
- [ ] 明确不保证独占全屏、GNOME Wayland 全屏 Overlay 和未验证 Gamescope 场景。

## 12. 决策结论

推荐采用以下路线：

> 共享 .NET Core + 保留 Windows WPF/Vortice 后端 + Avalonia Linux 设置 UI + X11 完整后端 + Wayland layer-shell 静态后端 + SDL3 手柄 + Portal/CLI 快捷键 + AppImage/deb/rpm/AUR/Flatpak 多渠道发布。

第一优先级不是打包，而是完成 **协议可行性原型和 Core 解耦**。如果没有这两步，任何 deb、rpm 或 AppImage 都只会把一个 Windows 专用程序包装成无法运行的 Linux 文件。

Linux 1.0 的合理产品承诺应是：

- 主流 Linux 发行版均可安装和启动。
- X11 提供完整功能。
- KDE Wayland 和支持 layer-shell 的 wlroots 提供可靠静态 Overlay，并按协议能力提供快捷键和手柄。
- GNOME Wayland 明确降级，不作无法兑现的置顶承诺。
- Gamescope/Steam Deck Gaming Mode 在实机验证完成前保持 Experimental。
