# SfxrVsti

<img src="docs/screenshot.png" alt="SfxrVsti 界面截图" width="760"/>

基于 [sfxr](http://www.drpetter.se/project_sfxr.html)（DrPetter, 2007）合成算法移植的 JUCE 乐器插件，将经典的「声音效果生成器」做成一个可被 DAW 自动化、支持复音与 MIDI 键盘的 VST3 / AU / Standalone 插件。

> [English version](README.en.md) · [用户手册](docs/user-manual.md) / [User Manual](docs/user-manual.en.md)

## 特性

- **忠实移植** sfxr 1.2.1 的合成核心（方波/锯齿/正弦/噪声 + LP/HP 滤波 + Phaser + 8x 过采样 + 重复触发 + 琶音）
- **复音**：8 个 voice，可切换 MONO 单音模式
- **MIDI 移调**：A4（note 69）对应「Start Frequency」旋钮的原值，其余音符按半音移调
- **两种触发模式**：One-Shot（默认，像鼓一样播完）与 Sustain（按住持续、松开释放）
- **全部 24 个 sfxr 参数** DAW 可自动化
- **经典 sfxr 外观**：米黄底 + 橙色滑条，7 个预设生成器 + RANDOMIZE / MUTATE
- **虚拟 MIDI 键盘**（全 88 键，有效区外灰化禁用，A4 高亮）+ 实时波形示波器
- **.sfs 文件兼容**：可读写原版 sfxr 的参数文件（版本 102）

## 目录结构

```
sfxr-vsti/
├── CMakeLists.txt          # 构建配置（FetchContent 拉取 JUCE）
├── Source/
│   ├── PluginProcessor.*   # 参数树、MIDI 调度、示波器缓冲
│   ├── PluginEditor.*      # GUI（滑条、键盘、示波器、预设按钮）
│   └── SfxrEngine/
│       ├── SfxrParams.h    # 参数定义（ID + 结构体）
│       ├── SfxrVoice.*     # 合成核心（ResetSample/SynthSample 移植）
│       ├── SfxrEngine.*    # 8 voice 池、MONO、note 调度
│       ├── SfxrPresets.*   # 7 类生成器 + RANDOMIZE + MUTATE
│       └── SfxrPresetFile.*# .sfs 文件读写
├── tests/RenderTest.cpp    # 离线渲染测试（验证 DSP）
├── scripts/build.sh        # 一键编译 + 安装（macOS/Linux）
└── reference/              # 上游 sfxr-sdl-1.2.1 源码（已 gitignore）
```

## 构建

依赖：CMake 3.24+、C++17 编译器。JUCE 8.0.15 通过 CMake `FetchContent` 在配置阶段自动下载（需联网）。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

或使用脚本：

```bash
./scripts/build.sh
```

### 各平台产物

| 平台 | 格式 | 产物位置 |
|------|------|----------|
| macOS | VST3 + AU + Standalone | `build/SfxrVsti_artefacts/Release/` |
| Windows | VST3 + Standalone | `build/SfxrVsti_artefacts/Release/` |
| Linux | VST3 + Standalone | `build/SfxrVsti_artefacts/Release/` |

Windows 需 MSVC 或 MinGW；Linux 需 ALSA/JACK 开发库（JUCE 处理链接）。

## 安装

`build.sh` 会在 macOS 上自动安装到用户插件目录：

- VST3 → `~/Library/Audio/Plug-Ins/VST3/`
- AU → `~/Library/Audio/Plug-Ins/Components/`

其他平台手动复制：
- Windows VST3 → `C:\Program Files\Common Files\VST3\`
- Linux VST3 → `~/.vst3/`

## 使用

### 界面

- **GENERATOR 列**（左）：7 个预设类别（PICKUP/COIN、LASER/SHOOT、EXPLOSION、POWERUP、HIT/HURT、JUMP、BLIP/SELECT）+ PLAY SOUND / RANDOMIZE / MUTATE / LOAD SOUND / SAVE SOUND
- **MANUAL SETTINGS**（右）：全部参数按 ENVELOPE / FREQUENCY / VIBRATO / SQUARE DUTY / REPEAT / ARPEGGIO / PHASER / FILTERS 分组
- **波形示波器**（底部）：实时显示输出波形
- **虚拟键盘**（底部）：88 键，点击/拖拽触发；A4 橙色高亮；C2–C6 之外灰化且不响应

### 有效音符范围

合成以 A4（note 69）为根音，其余音符按 `2^(-(note-69)/12)` 移调。有效触发范围为 **C2–C6（MIDI 36–84）**——更低基本听不见，更高易使噪声/爆炸类音效失真。范围外音符在 UI 上灰化、不响应鼠标，DAW MIDI 事件亦被忽略。

### 参数

| 参数 | 范围 | 说明 |
|------|------|------|
| Waveform | Square/Saw/Sine/Noise | 基础波形 |
| Attack / Sustain / Punch / Decay Time | 0–1 | 音量包络 |
| Start Frequency | 0–1 | 起始频率（根音 A4 对应此值） |
| Min Frequency | 0–1 | 频率下限（向下滑音触底即停止） |
| Slide / Delta Slide | -1–1 | 频率滑移 / 滑移变化率 |
| Vibrato Depth / Speed / Delay | 0–1 | 颤音 |
| Change Amount / Speed | -1–1 / 0–1 | 琶音（延时后跳变音高） |
| Square Duty / Duty Sweep | 0–1 / -1–1 | 方波占空比及其扫频 |
| Repeat Speed | 0–1 | 周期性重触发频率/占空比 |
| Phaser Offset / Sweep | -1–1 | 相位器（延时叠加） |
| LP Cutoff / Sweep / Resonance | 0–1 / -1–1 / 0–1 | 低通滤波器 |
| HP Cutoff / Sweep | 0–1 / -1–1 | 高通滤波器 |
| Output Level | 0–1 | 输出音量 |
| Mono | 开关 | 单音模式（新音符重触发） |
| One-Shot | 开关 | 关 = Sustain（按住持续） |

## 架构与移植说明

合成核心 `SfxrVoice` 是对原版 `ResetSample()` 与 `SynthSample()` 的逐句移植（见 `reference/sfxr-sdl-1.2.1/main.cpp`），主要改造：

- **采样率无关**：包络/重复/琶音/颤音等时间常数按 `sampleRate/44100` 缩放
- **MIDI 移调**：`fperiod *= 2^(-(note-69)/12)`，A4 对应原始参数
- **复音状态**：原版的全局变量全部下沉为 voice 实例字段
- **力度**：MIDI velocity 缩放输出增益
- **`vib_delay` 生效**：原版该参数从未使用，现实现为「延迟后淡入」
- **Sustain 模式**：One-Shot 关闭时，音符停在 Sustain 阶段直到 note-off

## 测试

`tests/RenderTest.cpp` 离线渲染引擎输出，验证波形电平与移调正确性：

```bash
cmake --build build --target SfxrRenderTest
./build/SfxrRenderTest_artefacts/Release/SfxrRenderTest
```

## 捐赠

如果这个插件对你有帮助，欢迎自愿捐赠支持开发：

<div align="center">
  <img src="docs/donate-wechat.png" alt="微信" width="200"/>
  <img src="docs/donate-alipay.png" alt="支付宝" width="200"/>
</div>

<p align="center">微信（左） · 支付宝（右）</p>

## 许可

本项目遵循 **GNU Affero General Public License v3（AGPLv3）** 开源许可。合成算法移植自 [sfxr](http://www.drpetter.se/project_sfxr.html)，其 MIT 许可声明予以保留。

- 许可全文：[LICENSE](LICENSE)
- 第三方组件声明：[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
