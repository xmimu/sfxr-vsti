# Changelog

本项目所有值得注意的变更都会记录在此文件中。

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [1.2.0] - 2026-09-04

### Fixed

- **wave_type 参数自动化可能在音频线程直接改 GUI**（原 P1）：`parameterChanged` 此前同步执行 `ToggleButton::setToggleState()`，宿主自动化该参数时存在 GUI 数据竞争并把控件工作带上实时线程；现 listener 只 `triggerAsyncUpdate()`，按钮刷新移到消息线程
- **CC120 All Sound Off 停不住 one-shot**（原 P2）：此前 CC120 与 CC123 都走正常 release，而 one-shot 忽略 note-off，宿主 panic / 停止后声音仍继续；新增 `SfxrEngine::allSoundOff()` 立即静音全部 voice，CC120 与 CC123 分开处理（CC123 保留正常 release）。新增渲染测试覆盖两条路径
- **超出 prepare 声明大小的 block 会在音频线程扩容**（原 P1）：`voiceBuffer.setSize()` 兜底可能实时分配/释放；现改为按预分配容量把超大请求分块渲染（`renderChunk`），音频线程不再触碰堆。新增「超大请求 == 分块渲染逐样本一致」回归测试
- **tail 长度仍低报**（原 P2）：One-Shot 的 attack+sustain+decay 会被宿主 bounce/freeze 截断；`getTailLengthSeconds()` 现报告三阶段平方和的保守上界（最大约 6.80 s）
- **`.sfs` 覆盖保存非事务性**（原 P2）：此前先把旧文件 truncate 再写入，磁盘满 / 中途失败会破坏原 preset；现写同目录临时文件、flush/close 成功后原子替换，任何失败路径保留原文件并清理临时文件
- **异步 FileChooser 回调捕获裸 `this`**（原 P1）：Load/Save 对话框打开期间关闭插件窗口会 use-after-free；回调改捕获 `Component::SafePointer` 并在入口校验
- **`.sfs` 只支持版本 102**：原版 sfxr 1.2.1 只写 102，加载端此前还接受更旧的 v100/v101 并按各自字段布局解码；现仅接受 102，旧档直接拒绝（返回失败且不修改目标参数），避免按错误布局错位读取历史存档
- **MONO 模式松开旧音符会误停正在响的新音**（legato 弹 C→D 后松开 C 直接掐掉 D）：MONO 现维护「仍按住音符」栈，按 last-note 优先级发声——松开更早按住的音符不再停掉当前声音，松开当前发声音时回落重触发仍按住的最新音符；CC All Notes Off / All Sound Off / 复位与模式切换同步清栈。新增回归测试
- **voice 抢占无淡出，超过 8 音时被抢 voice 波形硬切产生咔嗒**：抢占时先把被抢 voice 的尾音做约 3 ms 线性淡出，再切换到待播放的新音（`SfxrVoice::requestStealRestart`）；淡出期间若该音已被松开或触发全停，则安静收尾而不再重启。新增「200 轮抢占/释放风暴」回归测试
- **多 voice 混音总线可超过 0 dBFS**：每个 voice 已各自 clamp 到 ±1，但并发时直接相加会越界。现明确 headroom 契约——单个 voice 直通（输出与原版一致、不变），多个 voice 时按该时刻实际并发数 `1/N` 缩放总线，保证混音峰值不超满幅。新增「8 voice 满载和弦峰值 ≤ ±1」回归测试
- **乘法递推系数改用精确换算**：`fslide`/`fltw_d`/`flthp_d` 原先按一阶近似 `1 + delta/srScale` 缩放到采样率，极端 slide / filter sweep 在 22.05–192 kHz 下会相对 44.1 kHz 漂移约 0.2–3%；现改为把 44.1 kHz 基准系数取 `pow(基准值, 1/srScale)`，任意物理时长下的累乘结果与采样率无关，44.1 kHz 输出逐位不变

### Changed

- **编辑器模块重构（纯结构整理，观感与布局逐像素不变）**：`PluginEditor` 从 813 行瘦身为视图编排层，原先内嵌的示波器、屏幕键盘、主题 LookAndFeel、导出选项弹窗、弹窗辅助与业务动作分别拆入 `Source/Gui/`（`SfxrTheme` / `SfxrLookAndFeel` / `SfxrDialogs` / `WaveformScope` / `SfxrMidiKeyboard` / `ExportOptionsComponent` / `EditorActions`）；坐标、外观与交互保持不变，依赖方向固定为编辑器 → 动作层 → 引擎/参数
- **每 block 读参改为直接解引用缓存的原子指针**：构造函数一次性缓存全部 27 个参数的 `std::atomic<float>*`，`readParams()` 不再做 27 次字符串键查找；`ParamID` 常量由 `juce::String` 改为 `inline constexpr const char*`，消除每 TU 一份 String 及其静态初始化
- **批量预设写入对音频线程发布一致快照**：`applyParams()` 逐参写入期间音频线程可能拼出「半新半旧」的混合参数组。现先置「写入中」标志、完成后把整份参数提交到三重缓冲并以原子 generation 发布；音频线程只在 generation 变化时取整份快照，批次进行中沿用上一 block 的快照，单参数自动化仍逐参实时生效。批量 change gesture 也从逐参 begin/end 改为整批一次 begin、写入后统一 end
- `THIRD_PARTY_NOTICES.md` 不再声称上游源码保存在 `reference/`：`.gitignore` 忽略该目录、不随源码归档，声明改为「上游需从作者站点获取、本地副本仅用于对照审计」
- **LOAD SOUND / SAVE SOUND 重命名为 LOAD CONFIG / SAVE CONFIG**：更准确地反映 `.sfs` 保存的是参数配置而非音频本身
- Standalone 窗口改用原生标题栏，并固定初始内容尺寸为 880×700
- **弹窗风格统一**：Load/Save/Export 失败提示与「文件已存在」确认弹窗此前使用 JUCE 默认的深色 LookAndFeel，与其余界面的 sfxr 米色主题不一致；现改为手动构建 `AlertWindow` 并显式套用同一份 LookAndFeel。`SfxrLookAndFeel` 此前未指定 `AlertWindow::backgroundColourId` / `textColourId`，套用后弹窗背景仍是 `LookAndFeel_V4` 的深色底，现补齐这两个颜色 ID
- **GENERATOR 列按钮重新分组排序**：MUTATE / RANDOMIZE / PLAY SOUND 为一组，LOAD CONFIG / SAVE CONFIG / EXPORT AUDIO 为另一组，两组之间加入分隔线与间距
- **补齐效果特性的跨采样率测试**（原测试缺口）：新增综合属性测试，以 44.1 kHz 为基准在 5 种采样率下断言——arpeggio 单步频率比（解析期望值）、repeat 驱动下相邻探测窗的音高轨迹一致、LP/HP filter sweep 与 phaser 的零交叉率（频谱亮度代理）一致、vibrato 延迟淡入之后的调频幅度远大于淡入前
- **构建与 CI 加固**：插件/测试目标启用 JUCE recommended config/warning/lto flags 并关闭 web/cURL；新增 `SfxrPluginTest` 处理器级集成测试（真实 `AudioProcessor` 的 note 发声、CC120 立即静音、超大 block、确定性 program 与 state 往返，CTest 接入）；打包统一为各平台单归档（macOS zip、Linux tar.gz 保可执行权限、Windows zip），并把 `LICENSE` / `THIRD_PARTY_NOTICES.md` / `SOURCE.txt` 随归档发布、Release 阶段自动校验；CI 清理中间文件改跨平台 Python；macOS CI 用 `auvaltool` 实际加载校验 AU bundle
- **JUCE 依赖固定到 commit**：`FetchContent` 从 tag `8.0.15` 改为其不可变的完整 commit `91ad83a`，CI 的 JUCE 缓存 key 同步更新，避免依赖在 tag 被移动时静默变化
- **构建环境全部固定**：GitHub Actions（checkout/cache/upload-artifact/download-artifact/action-gh-release）改用不可变 commit SHA；runner 固定 macOS 14 / Windows Server 2022 / Ubuntu 22.04；macOS 部署目标固定 11.0（CMake 内建默认 + CI 参数），Linux 最低 glibc 2.35 已声明
- **收尾加固**：`currentProgram` 改 `std::atomic<int>`（消除宿主并发查询/切换/恢复 state 的数据竞争）；`build.sh` 拆为「只构建」+ 新 `scripts/install.sh`（macOS 安装，失败即非零、成功才报 Installed）；CI Release 校验 git tag 与 `project(VERSION)` 一致；Windows/Linux 加 `pluginval` v1.0.4 真实 VST3 宿主扫描（Linux 走 xvfb）；新增 `SfxrGuiTest`（开合编辑器 15 次 + LeakedObjectDetector）让「77 组件泄漏」与「参数不被量化」两条都有常驻回归

### Added

- **Standalone 音频导出**：新增 EXPORT AUDIO 按钮（仅 Standalone 显示），将当前声音离线渲染为 WAV（16/24-bit PCM、32-bit float）或 OGG（可选比特率），支持 44.1/48/88.2/96/192 kHz 采样率。渲染以根音（MIDI note 69）满力度触发，输出单声道；One-Shot 声音渲染到自然结束，Sustain 声音固定导出 10 秒以避免无界文件。格式、采样率、位深/比特率与导出目录会记录到用户设置文件中，下次打开对话框自动带出上次的选择；目标文件已存在时会先弹窗确认是否替换

## [1.1.0] - 2026-08-28

### Fixed

- **RANDOMIZE / MUTATE 与原版 sfxr 的边界行为不一致**：随机公式会产生超出控件范围的值，而原版会在同一 UI 帧中执行所有可见 `Slider()`，于 `PlaySample()` 前把单极参数夹到 0–1、双极参数夹到 -1–1。现新增显式 `clampToDomain()` 复现该行为，不再把负的单极参数错误地取绝对值；随机浮点也恢复为原版的 10001 个离散点并包含上下界。`vib_delay` 继续按插件扩展的 0–1 范围处理
- **参数被量化到 0.01 步进**：`AudioParameterFloat` 的便捷构造函数等价于 `{ min, max, 0.01f }`，24 个浮点参数全部被吸附到 1%。sfxr 里这些都是连续浮点，所以这是保真度问题而非观感问题——加载原版 `.sfs` 时 `base_freq = 0.4372` 会被吸附成 `0.44`，声音与原版不一致；MUTATE 每次只推 ±0.05，五分之一的精度被丢掉。现显式指定 `NormalisableRange` 不做吸附（既有的 `.sfs` 测试抓不到这个问题，因为它直接测 `SfxrPresetFile`，而量化发生在更后面的参数树环节）
- **MIDI 事件被量化到 block 边界**：所有事件在渲染前一次性处理完，512 采样缓冲下约 11 ms 抖动。现按事件位置切分渲染，音符起始精确到采样
- **音高与采样率挂钩**：`fperiod` 未按 `sampleRate/44100` 缩放，导致 48 kHz 下整体偏高约 1.47 个半音、96 kHz 下偏高约 13.5 个半音（48 kHz 是多数 DAW 的默认值，等于默认场景音高全错）
- **颤音速率缩放方向反了**：`vib_speed` 是每输出采样的相位增量，应除以 `srScale` 而不是乘，导致误差为 `srScale²`（96 kHz 下快 4 倍）
- **滑音 / 占空比扫描 / 滤波器扫描 / phaser 延时同样与采样率挂钩**：现按各自量纲统一缩放（长度 × `srScale`、一阶速率 ÷ `srScale`、二阶速率 ÷ `srScale²`）；phaser 延迟线随之从 1024 扩到 8192 以在高采样率下保住相同延迟时间
- **NaN 泄漏到音频输出**：包络阶段长度为 0（例如 DECAY TIME 拉到 0）时 `0/0` 产生 NaN，而 `if (s > 1.0f)` 形式的限幅对 NaN 恒为 false，拦不住。现改为在除法处判零（而非给长度加下限），因此非退化的声音与原版保持逐样本一致
- **voice 被抢占后陈旧的 note→voice 映射会误释放其他音符**：Sustain 模式下同时按住超过 8 个音时，松开旧音符会提前掐掉正在按住的新音符；改为维护 note↔voice 双向映射，抢占/结束时立即失效旧条目
- **音频线程每 block 堆分配**：`SfxrEngine::process` 每个 block 都构造一个临时 `AudioBuffer`；改为在 `prepare()` 中按 `samplesPerBlock` 预分配。宿主提交超出声明大小的 block 时仍有后备扩容路径，后续将改为固定缓冲分段渲染
- **编辑器泄漏约 77 个 Component**：滑条/标签/按钮全部 `new` 后只 `addAndMakeVisible`（该函数不接管所有权，`Component` 析构也不删子组件），每次开关插件窗口泄漏一份；改由 `OwnedArray` 持有
- **保存 .sfs 时追加而非覆盖**：`FileOutputStream` 默认定位到文件末尾，覆盖已有文件时会在后面追加一条记录，而 `load()` 只读开头——即另存到已存在的文件后读回的仍是旧声音。现显式 `setPosition(0)` + `truncate()`，并检查写入状态（此前 `save()` 无论成败恒返回 true，"Save Failed" 提示形同虚设）
- `getTailLengthSeconds()` 恒返回 0，导致宿主在导出/冻结时截断 decay 尾音；现至少按 DECAY TIME 上报。One-Shot 的完整 attack+sustain+decay 上界仍需后续补齐

### Changed

- **引擎改为纯音频线程驱动，移除 `CriticalSection`**：此前 `process()` 全程持锁，而屏幕键盘点击会从消息线程经监听器抢同一把锁，音频线程可被 UI 阻塞；`processBlock` 还会在音频线程上抢 `MidiKeyboardState` 自己的锁，且 `processNextMidiEvent` 不加锁地改 `noteStates[]`，与消息线程构成数据竞争。现改用 JUCE 惯用路径 `processNextMidiBuffer(..., injectIndirectEvents = true)`，UI 音符与宿主音符走同一条路，参数按值传入，锁彻底消失（`MidiKeyboardState` 合并时仍有它自身的短锁，这是 JUCE 的既有设计，区别在于不再横跨整个渲染）。有效音符范围过滤随之移入新的 MIDI 分发点，note-on 与 note-off 成对过滤（不会出现「没启动却被释放」），all-notes-off 不过滤
- **新增 8 个出厂 program**（Init + 7 个生成器类别）暴露给宿主预设菜单。刻意做成确定性（按索引固定种子）：宿主恢复工程时可能调用 `setCurrentProgram`，若结果随机就会静默覆盖用户保存的参数。状态恢复直接套用已保存参数、只记住索引，不重跑生成器；RANDOMIZE 与界面上的类别按钮仍保持随机
- 重新逐行核对 sfxr 1.2.1：确认 oscillator、filter、phaser、repeat、arpeggio 与 envelope 的处理顺序一致；噪声随机值恢复为原版离散值域；类别生成器不再错误重置 Output Level。实现继续使用现代浮点写法，不追求平台相关的逐位输出或 `rand()` 序列一致
- 保留 `vib_delay` 扩展：原版会保存和随机化该字段但不使用，本插件将其解释为颤音从零到完整深度的渐入时间
- 测试从「打印数值」改为**带断言的引擎测试套件**（134 项检查），并通过 `enable_testing()` / `add_test` 接入 CTest，CI 每次构建都会运行。覆盖 44.1/48/88.2/96/192 kHz 的音高、包络、颤音、温和滑音、占空比一致性，692 组参数的单 voice NaN/限幅扫描，原版 Slider clamp 语义与 10850 组生成参数的取值域校验，`.sfs` v102 全参数往返、NaN/损坏文件拒绝，以及音符起始的采样级精度和 voice 抢占后不误释放其他音符
- 虚拟键盘上 note 69 的标签从「A4」改为「ROOT」：它播放的是 START FREQ 的原值（默认约 321 Hz），并非 440 Hz 的标准 A4，旧标签会误导
- 预设 / RANDOMIZE / MUTATE / 波形切换现通过 `beginChangeGesture`/`endChangeGesture` 通知宿主；批量预设写入目前仍是逐参数 gesture，并非单一事务
- CI：运行 `ctest`；缓存 JUCE 的 FetchContent 克隆；macOS 产出 arm64 + x86_64 通用二进制并做 ad-hoc 签名及结构验证，改用 `ditto` 打包以保住 bundle 结构。产物未经 Apple notarization，文档已明确说明并提供 quarantine 处理方式
- `prepareToPlay` 现会重置引擎与示波器 FIFO，`releaseResources` 会停掉所有 voice
- 许可从 MIT 变更为 **AGPLv3**（因使用 JUCE 免费许可）；sfxr 的 MIT 声明移至 `THIRD_PARTY_NOTICES.md`
- 更正 README / 用户手册中与实现不符或超过测试范围的说法：参数在 note-on 时锁存；屏幕键盘显示 A0–C8，但插件只响应 C2–C6（MIDI 36–84）；note 69 是 START FREQ 根音而非标准 A4；跨采样率、0 dBFS 与 `.sfs` 往返说明现明确限定到实际测试范围
- 引擎的 NaN/限幅扫描扩大到完整 0–127 音域并显式覆盖极端值（note 0/1/12/24/100/120/126/127 × 44.1 与 96 kHz）：虽然插件只放行 C2–C6，但 `SfxrVoice` 的接口接受任意音符，这层防御性覆盖可避免日后改动引入退化

### Added

- 界面左上角显示由构建元数据生成的插件版本号，避免 UI 与发布版本不一致
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
