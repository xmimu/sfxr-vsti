# Changelog

本项目所有值得注意的变更都会记录在此文件中。

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### Added

- GitHub Actions 三平台构建（macOS/Windows/Linux）+ tag 触发 Release 自动发布预编译产物
- Windows 构建脚本 `scripts/build_windows.bat`
- 捐赠二维码（`docs/donate-wechat.png` / `docs/donate-alipay.png`）

### Changed

- 许可从 MIT 变更为 **AGPLv3**（因使用 JUCE 免费许可）；sfxr 的 MIT 声明移至 `THIRD_PARTY_NOTICES.md`
- 新增用户手册（`docs/user-manual.md` / `docs/user-manual.en.md`）
- README 与用户手册增加「下载预编译产物」与「从源码构建」说明

## [1.0.0] - 2026-08-27

### Added

- 移植 sfxr 1.2.1 合成核心（`SfxrVoice`）：方波/锯齿/正弦/噪声、LP/HP 滤波、1024 点 phaser、8x 过采样、repeat、arpeggio
- 8 复音引擎（`SfxrEngine`）+ MONO 单音模式
- 全部 24 个 sfxr 参数（DAW 可自动化），外加 Output Level / Mono / One-Shot
- MIDI 移调：A4（note 69）对应 Start Frequency 原值
- 两种触发模式：One-Shot（默认）与 Sustain（note-off 释放）
- 补齐原版从未使用的 `vib_delay`（延迟后淡入）
- 采样率无关的时间常数缩放（`sampleRate/44100`）
- 7 个预设生成器 + RANDOMIZE + MUTATE（移植自原版 UI）
- `.sfs` 参数文件读写（版本 102，字节兼容）
- 经典 sfxr 外观 GUI（米黄/橙色滑条 + 分组布局）
- 虚拟 MIDI 键盘（全 88 键，有效区 C2–C6 外灰化禁用，A4 橙色高亮 + "A4" 标签）
- 实时波形示波器（lock-free FIFO，音频线程→GUI 线程）
- 有效音符范围过滤（`SfxrNoteRange`），UI 与 DAW MIDI 统一生效
- VST3 + AU + Standalone 构建目标（按平台条件化格式）
- 离线渲染测试 `tests/RenderTest.cpp`

### Fixed

- 生成按钮（category/RANDOMIZE/MUTATE）播放时残留上一类型参数的问题——`playPreview` 现在发声前同步引擎参数
- 虚拟键盘点击不发声——补齐 `MidiKeyboardState.addListener` 注册
- 同音符重复触发导致 voice 泄漏（孤儿 voice 无法释放）
- 音频线程与消息线程并发访问引擎的数据竞态（补锁 `setParams`/`setMono`/`setOneShot`）
- 多 voice 噪声波形完全相关（每个 voice 独立随机种子）

### Changed

- 输出增益对齐原版 WAV 导出电平（`kOutputGain = 0.2`）
- 上游 sfxr-sdl-1.2.1 源码移至 `reference/` 并加入 `.gitignore`
- `build.sh` 跨平台化（`nproc`/`getconf` 取核数，安装逻辑按平台条件化）
