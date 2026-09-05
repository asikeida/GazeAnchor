# Linux 场景验证清单

此清单用于验证 KDE Wayland、X11 fallback、Steam/Gamescope 和游戏窗口下的实际叠加效果。

## 1. 基础环境

```bash
bash scripts/check-env.sh
/tmp/gaze-anchor-build/gaze-anchor --diagnose
```

期望：

- `XDG_CURRENT_DESKTOP=KDE`
- KDE/LayerShellQt/KGlobalAccel 依赖均为 `[ok]`
- 屏幕数量、几何尺寸、DPR 与系统设置一致

## 2. KDE Wayland 桌面

运行：

```bash
/tmp/gaze-anchor-build/gaze-anchor
```

检查：

- 每个显示器都有叠加层。
- 鼠标点击能穿透叠加层。
- F1 开关边缘叠加。
- F2 开关准星。
- F3 开关时钟。
- 托盘菜单可显示、隐藏设置窗口和退出。
- Profiles 页可保存、加载、删除方案。
- 语言下拉框可切换 English/中文。

## 3. KDE 缩放比例

在系统设置中分别测试：

- 100%
- 125%
- 150%
- 200%

检查：

- Overlay 覆盖完整屏幕。
- 边缘叠加位于正确屏幕边界。
- 准星位于目标区域中心。
- `--diagnose` 中 DPR 与系统设置一致。

## 4. 游戏窗口模式

建议先测试无边框窗口和窗口模式。

检查：

- 设置窗口隐藏后 Overlay 仍显示。
- 游戏可正常接收鼠标和键盘输入。
- F1/F2/F3 在游戏获得焦点时仍能触发。
- 边缘叠加没有被游戏窗口遮挡。

## 5. 独占全屏

Wayland/KDE 下独占全屏行为取决于 compositor 和游戏运行方式。

检查：

- Overlay 是否仍在游戏上方。
- 如果不可见，切换游戏到无边框窗口或窗口模式。
- 记录游戏名、启动参数、显示服务器、是否使用 Proton。

## 6. Steam / Proton

建议测试启动参数：

```text
%command%
```

以及无边框/窗口模式。

检查：

- Steam Overlay 与 Motion Stabilizer Overlay 是否冲突。
- Proton 游戏获得焦点时 F1/F2/F3 是否生效。
- 游戏退出后 Overlay 是否仍正常。

## 7. Gamescope

建议分别测试：

```bash
gamescope -W 1920 -H 1080 -- /tmp/gaze-anchor-build/gaze-anchor
```

以及从 Steam 中使用 gamescope 包裹游戏。

检查：

- Overlay 是显示在 gamescope 外层还是内层。
- F1/F2/F3 是否被 gamescope 捕获。
- 鼠标点击是否穿透到游戏。

## 8. X11 fallback

在 X11 会话中运行：

```bash
echo $XDG_SESSION_TYPE
/tmp/gaze-anchor-build/gaze-anchor --diagnose
/tmp/gaze-anchor-build/gaze-anchor
```

检查：

- `Qt platform=xcb`
- 日志显示 `Configuring X11 overlay fallback`
- Overlay 置顶。
- 鼠标点击穿透。

## 9. 问题记录模板

```text
系统：
桌面会话：
Qt platform：
屏幕数量/DPR：
游戏：
启动方式：
窗口模式：
是否 Proton：
是否 Gamescope：
现象：
复现步骤：
--diagnose 输出：
```
