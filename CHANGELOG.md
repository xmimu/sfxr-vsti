# Changelog

本项目所有值得注意的变更都会记录在此文件中。

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### Fixed

- **RANDOMIZE / MUTATE 的结果与原版 sfxr 不一致**：原版的随机化表达式会越出自己声明的取值域（例如 `p_env_decay = frnd(2) - 1`，而 `env_decay` 概念上是 0–1），这些值此前被参数树直接夹到 0。但原版合成核心把大多数单极性参数**平方**使用，因此负值听起来与其绝对值完全相同，夹到 0 反而等于「关掉该效果」——实测 20000 次采样中，**97.1% 的 RANDOMIZE 结果至少有一个参数被压掉**（`env_decay` / `env_attack` / `vib_speed` / `vib_delay` / `lpf_resonance` 各约 50%，衰减、起音、颤音、共振会直接消失）。新增 `SfxrParams::foldIntoDomain()`，按参数在合成器中的实际用法分三类收敛：平方使用的取 `abs()`（**精确等价**，测试逐位比对渲染输出加以证明）、夹取恰好等价的（`duty` / `vib_strength` / `lpf_freq`）保持夹取、无法表示的（`base_freq > 1`、负 `env_punch`、负 `repeat_speed`）取最近值并在注释中写明差异。`.sfs` 载入改用同一套折叠——原版会把越界值写进文件，因此这同时提升了真实 `.sfs` 文件的还原度
- **参数被量化到 0.01 步进**：`AudioParameterFloat` 的便捷构造函数等价于 `{ min, max, 0.01f }`，24 个浮点参数全部被吸附到 1%。sfxr 里这些都是连续浮点，所以这是保真度问题而非观感问题——加载原版 `.sfs` 时 `base_freq = 0.4372` 会被吸附成 `0.44`，声音与原版不一致；MUTATE 每次只推 ±0.05，五分之一的精度被丢掉。现显式指定 `NormalisableRange` 不做吸附（既有的 `.sfs` 测试抓不到这个问题，因为它直接测 `SfxrPresetFile`，而量化发生在更后面的参数树环节）
- **MIDI 事件被量化到 block 边界**：所有事件在渲染前一次性处理完，512 采样缓冲下约 11 ms 抖动。现按事件位置切分渲染，音符起始精确到采样
- **音高与采样率挂钩**：`fperiod` 未按 `sampleRate/44100` 缩放，导致 48 kHz 下整体偏高约 1.47 个半音、96 kHz 下偏高约 13.5 个半音（48 kHz 是多数 DAW 的默认值，等于默认场景音高全错）
- **颤音速率缩放方向反了**：`vib_speed` 是每输出采样的相位增量，应除以 `srScale` 而不是乘，导致误差为 `srScale²`（96 kHz 下快 4 倍）
- **滑音 / 占空比扫描 / 滤波器扫描 / phaser 延时同样与采样率挂钩**：现按各自量纲统一缩放（长度 × `srScale`、一阶速率 ÷ `srScale`、二阶速率 ÷ `srScale²`）；phaser 延迟线随之从 1024 扩到 8192 以在高采样率下保住相同延迟时间
- **NaN 泄漏到音频输出**：包络阶段长度为 0（例如 DECAY TIME 拉到 0）时 `0/0` 产生 NaN，而 `if (s > 1.0f)` 形式的限幅对 NaN 恒为 false，拦不住。现改为在除法处判零（而非给长度加下限），因此非退化的声音与原版保持逐样本一致
- **voice 被抢占后陈旧的 note→voice 映射会误释放其他音符**：Sustain 模式下同时按住超过 8 个音时，松开旧音符会提前掐掉正在按住的新音符；改为维护 note↔voice 双向映射，抢占/结束时立即失效旧条目
- **音频线程堆分配**：`SfxrEngine::process` 每个 block 都构造一个临时 `AudioBuffer`；改为在 `prepare()` 中按 `samplesPerBlock` 预分配
- **编辑器泄漏约 77 个 Component**：滑条/标签/按钮全部 `new` 后只 `addAndMakeVisible`（该函数不接管所有权，`Component` 析构也不删子组件），每次开关插件窗口泄漏一份；改由 `OwnedArray` 持有
- **保存 .sfs 时追加而非覆盖**：`FileOutputStream` 默认定位到文件末尾，覆盖已有文件时会在后面追加一条记录，而 `load()` 只读开头——即另存到已存在的文件后读回的仍是旧声音。现显式 `setPosition(0)` + `truncate()`，并检查写入状态（此前 `save()` 无论成败恒返回 true，"Save Failed" 提示形同虚设）
- `getTailLengthSeconds()` 恒返回 0，导致宿主在导出/冻结时截断 decay 尾音；现按 DECAY TIME 上报

### Changed

- **引擎改为纯音频线程驱动，移除 `CriticalSection`**：此前 `process()` 全程持锁，而屏幕键盘点击会从消息线程经监听器抢同一把锁，音频线程可被 UI 阻塞；`processBlock` 还会在音频线程上抢 `MidiKeyboardState` 自己的锁，且 `processNextMidiEvent` 不加锁地改 `noteStates[]`，与消息线程构成数据竞争。现改用 JUCE 惯用路径 `processNextMidiBuffer(..., injectIndirectEvents = true)`，UI 音符与宿主音符走同一条路，参数按值传入，锁彻底消失（`MidiKeyboardState` 合并时仍有它自身的短锁，这是 JUCE 的既有设计，区别在于不再横跨整个渲染）。有效音符范围过滤随之移入新的 MIDI 分发点，note-on 与 note-off 成对过滤（不会出现「没启动却被释放」），all-notes-off 不过滤
- **新增 8 个出厂 program**（Init + 7 个生成器类别）暴露给宿主预设菜单。刻意做成确定性（按索引固定种子）：宿主恢复工程时可能调用 `setCurrentProgram`，若结果随机就会静默覆盖用户保存的参数。状态恢复直接套用已保存参数、只记住索引，不重跑生成器；RANDOMIZE 与界面上的类别按钮仍保持随机
- 测试从「打印数值」改为**带断言的测试套件**（92 项检查），并通过 `enable_testing()` / `add_test` 接入 CTest，CI 每次构建都会运行。覆盖 44.1/48/88.2/96/192 kHz 的音高、包络、颤音、滑音、占空比一致性，692 组参数的 NaN/限幅扫描，越界值折叠的逐位等价性与 10850 组生成参数的取值域校验，`.sfs` 往返与损坏文件拒绝，以及音符起始的采样级精度和 voice 抢占后不误释放其他音符
- 虚拟键盘上 note 69 的标签从「A4」改为「ROOT」：它播放的是 START FREQ 的原值（默认约 321 Hz），并非 440 Hz 的标准 A4，旧标签会误导
- 预设 / RANDOMIZE / MUTATE / 波形切换现包在 `beginChangeGesture`/`endChangeGesture` 中，宿主会把整次改动视为一个编辑动作
- CI：运行 `ctest`；缓存 JUCE 的 FetchContent 克隆；macOS 产出 arm64 + x86_64 通用二进制并做 ad-hoc 签名，改用 `ditto` 打包以保住 bundle 结构
- `prepareToPlay` 现会重置引擎与示波器 FIFO，`releaseResources` 会停掉所有 voice
- 许可从 MIT 变更为 **AGPLv3**（因使用 JUCE 免费许可）；sfxr 的 MIT 声明移至 `THIRD_PARTY_NOTICES.md`
- 更正 README / 用户手册中与实现不符的说法：「采样率无关」此前并不成立、参数其实在 note-on 时锁存（自动化不影响已发声音符）。用户手册中「低于 C2 约 6 Hz」的数字为误：实测默认 START FREQ (0.3) 下 C2 约 48 Hz、C6 约 764 Hz，且该频率随 START FREQ 平移，因此 C2–C6 是「可用音域约定」而非技术边界，文档已按此改写
- 引擎的 NaN/限幅扫描扩大到完整 0–127 音域并显式覆盖极端值（note 0/1/12/24/100/120/126/127 × 44.1 与 96 kHz）：虽然插件只放行 C2–C6，但 `SfxrVoice` 的接口接受任意音符，这层防御性覆盖可避免日后改动引入退化

### Added

- GitHub Actions 三平台构建（macOS/Windows/Linux）+ tag 触发 Release 自动发布预编译产物
- Windows 构建脚本 `scripts/build_windows.bat`
- 用户手册（`docs/user-manual.md` / `docs/user-manual.en.md`）
- 捐赠二维码（`docs/donate-wechat.png` / `docs/donate-alipay.png`）

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
