# GazeAnchor 发布操作手册

> 适用仓库：当前 `GazeAnchor` 主仓库  
> 目标：提供一套从改版本到发 GitHub Release 的固定操作流程。

## 1. 先知道现在有哪些发布入口

当前仓库已经有这些脚本和流程：

- 版本读取：`packaging/version.sh`
- 版本准备：`packaging/prepare-release-version.sh`
- Debian 构建：`packaging/build-deb-package.sh`
- Debian 验包：`packaging/verify-deb-package.sh`
- RPM 构建：`packaging/build-rpm-package.sh`
- RPM 验包：`packaging/verify-rpm-package.sh`
- AppDir / AppImage staging：`packaging/appimage-build.sh`
- 源码包：`packaging/build-source-tarball.sh`
- release 产物收集：`packaging/collect-release-artifacts.sh`
- release notes：`packaging/generate-release-notes.sh`
- release 一致性检查：`packaging/verify-release-consistency.sh`
- 日常 CI：`.github/workflows/linux-ci.yml`
- tag 发布 CI：`.github/workflows/release.yml`

## 2. 平时开发时怎么做

改功能时，不需要先考虑打包。

正常顺序：

1. 改源码。
2. 本地构建。
3. 本地测试。
4. 确认功能没问题后，再考虑是否要发新版本。

常用命令：

```bash
cmake -S . -B /tmp/gaze-anchor-build -DBUILD_PROFILE=FULL -DBUILD_TESTING=ON
cmake --build /tmp/gaze-anchor-build -j
ctest --test-dir /tmp/gaze-anchor-build --output-on-failure
bash scripts/check-env.sh
```

## 3. 准备发新版本时怎么做

假设这次要发 `0.2.0`。

先统一更新版本相关文件：

```bash
bash packaging/prepare-release-version.sh 0.2.0 2026-09-05
```

这个脚本会更新：

- `CMakeLists.txt`
- `README.md` 顶部版本
- `packaging/io.github.gazeanchor.GazeAnchor.metainfo.xml`
- `packaging/gaze-anchor.spec`
- `debian/changelog`

然后检查当前版本：

```bash
bash packaging/version.sh project-version
bash packaging/version.sh release-version
bash packaging/version.sh release-prefix
```

理想输出应类似：

```text
0.2.0
0.2.0
gaze-anchor-0.2.0
```

## 4. 发版前本地检查顺序

推荐按这个顺序做：

1. 环境检查

```bash
bash scripts/check-env.sh
```

2. 完整构建与测试

```bash
cmake -S . -B /tmp/gaze-anchor-build -DBUILD_PROFILE=FULL -DBUILD_TESTING=ON
cmake --build /tmp/gaze-anchor-build -j
ctest --test-dir /tmp/gaze-anchor-build --output-on-failure
```

3. Debian 包

```bash
bash packaging/build-deb-package.sh
bash packaging/verify-deb-package.sh
```

4. RPM 包

```bash
bash packaging/build-rpm-package.sh
bash packaging/verify-rpm-package.sh
```

5. AppDir staging

```bash
bash packaging/appimage-build.sh
```

6. 收集 release 产物

```bash
bash packaging/collect-release-artifacts.sh
```

7. 做 release 一致性检查

```bash
bash packaging/verify-release-consistency.sh 0.2.0
```

## 5. 当前产物默认会出到哪里

### Debian

- `.deb`
- `.changes`
- `.buildinfo`

默认在仓库父目录。

### RPM

- `.rpm`
- `.src.rpm`

默认在：

```text
/tmp/opencode/gaze-anchor-rpmbuild/
```

### AppDir

默认在：

```text
/tmp/GazeAnchor.AppDir
```

### release 汇总目录

运行：

```bash
bash packaging/collect-release-artifacts.sh
```

后，会整理到：

```text
dist/release/
```

里面通常会有：

- `source/gaze-anchor-<version>.tar.gz`
- `deb/*`
- `rpm/*`
- `appimage/*`
- `RELEASE-NOTES.md`
- `SHA256SUMS`

## 6. 什么时候打 tag

只有在下面这些都通过后再打 tag：

1. 版本已经更新。
2. 功能已经验证。
3. `deb` 和 `rpm` 已经能构建。
4. `collect-release-artifacts.sh` 已经跑过。
5. `verify-release-consistency.sh <version>` 已通过。

然后再打 tag：

```bash
git tag v0.2.0
git push origin v0.2.0
```

## 7. tag 推上去后会发生什么

`.github/workflows/release.yml` 会触发。

它现在会做：

1. 构建源码 tarball。
2. 构建并校验 `.deb`。
3. 构建并校验 `.rpm`。
4. 构建 AppDir staging。
5. 收集 release 文件。
6. 生成 `RELEASE-NOTES.md`。
7. 生成 `SHA256SUMS`。
8. 做 release 一致性检查。
9. 创建 GitHub Release 并上传产物。

## 8. release 一致性检查到底在查什么

`packaging/verify-release-consistency.sh` 当前会检查：

- 项目版本是否等于预期版本
- tag 版本是否等于预期版本
- `RELEASE-NOTES.md` 是否存在
- `SHA256SUMS` 是否存在
- 源码 tarball 是否存在
- `deb/`、`rpm/`、`appimage/` 至少有一种交付产物
- 产物文件名是否带版本号
- `SHA256SUMS` 是否覆盖全部 release 文件
- `sha256sum -c SHA256SUMS` 是否通过

## 9. 当前已知限制

这几个点现在要明确记住：

1. `deb` 和 `rpm` 当前默认走 `BUILD_PROFILE=X11`。
2. KDE Wayland 的完整原生包支持还需要继续补目标发行版依赖。
3. AppImage 还没有完全进入正式自包含发布链。
4. 当前 release 更像是：源码包 + deb + rpm + AppDir archive。
5. 真正可移植的 `.AppImage` 还需要 `linuxdeployqt` / `appimagetool` 完整接入。

## 10. 最推荐的固定发布顺序

以后建议固定按下面执行：

```text
1. 修改功能
2. 本地构建和测试
3. prepare-release-version.sh
4. build-deb-package.sh
5. verify-deb-package.sh
6. build-rpm-package.sh
7. verify-rpm-package.sh
8. appimage-build.sh
9. collect-release-artifacts.sh
10. verify-release-consistency.sh <version>
11. 提交发布相关改动
12. 打 tag: v<version>
13. 推送 tag
14. 等 GitHub Release workflow 完成
15. 下载并复核最终产物
```

## 11. 最后下载完 release 后再看什么

至少检查：

1. GitHub Release 页面是否包含预期文件。
2. `SHA256SUMS` 是否存在。
3. `RELEASE-NOTES.md` 是否存在。
4. 文件名里的版本号是否正确。
5. 随机抽一个产物做安装或启动验证。

## 12. 如果只是改了一个小功能怎么办

流程不变，只是规模变小。

你仍然是：

1. 改代码
2. 测试
3. 如果要发布，就升版本
4. 重新打包
5. 发一个新 tag

打包不是把项目“封死”，只是把某个版本整理成可分发产物。
