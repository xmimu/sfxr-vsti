# SfxrVsti

<img src="docs/screenshot.png" alt="SfxrVsti 界面截图" width="760"/>

基于 [sfxr](http://www.drpetter.se/project_sfxr.html)（DrPetter, 2007）合成算法移植的 JUCE 乐器插件，将经典的「声音效果生成器」做成一个可被 DAW 自动化、支持复音与 MIDI 键盘的 VST3 / AU / Standalone 插件。

> [English version](README.en.md) · [用户手册](docs/user-manual.md) / [User Manual](docs/user-manual.en.md)

## 特性

- **忠实移植** sfxr 1.2.1 的合成核心（方波/锯齿/正弦/噪声 + LP/HP 滤波 + Phaser + 8x 过采样 + 重复触发 + 琶音）
- **复音**：8 个 voice，可切换 MONO 单音模式
- **MIDI 移调**：note 69 为根音，对应「Start Frequency」旋钮的原值，其余音符按半音移调（根音的实际频率由 Start Frequency 决定，并非 440 Hz 的标准 A4）
- **两种触发模式**：One-Shot（默认，像鼓一样播完）与 Sustain（按住持续、松开释放）
- **全部 24 个 sfxr 参数** 均暴露给宿主（注意：参数在 note-on 时锁存，与原版 sfxr 一致，自动化不会改变已发声音符）
- **经典 sfxr 外观**：米黄底 + 橙色滑条，7 个预设生成器 + RANDOMIZE / MUTATE
- **虚拟 MIDI 键盘**：显示 A0–C8 共 88 键，但插件只响应 C2–C6（MIDI 36–84）；范围外按键灰化禁用，根音高亮
- **.sfs 文件兼容**：可读取原版 sfxr 100/101/102 格式并写出 102 格式，参数为连续浮点、不做量化
- **8 个出厂 program**：Init + 7 个生成器类别，暴露给宿主的预设菜单（确定性，选中同一个 program 始终得到同一个声音）

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
├── scripts/build.sh        # 构建脚本（macOS/Linux）
├── scripts/build_windows.bat # 构建脚本（Windows）
└── reference/              # 可选的本地上游参考源码（已 gitignore，不属于仓库内容）
```

## 下载

从 [GitHub Releases](../../releases) 下载对应平台的预编译产物（由 GitHub Actions 自动构建）：

| 平台 | 文件 | 包含格式 |
|------|------|----------|
| macOS | `SfxrVsti-macOS.zip` | VST3 + AU + Standalone |
| Windows | `SfxrVsti-Windows.zip` | VST3 + Standalone |
| Linux | `SfxrVsti-Linux.zip` | VST3 + Standalone |

## 安装

### macOS

解压后复制到用户插件目录：

- VST3 → `~/Library/Audio/Plug-Ins/VST3/`
- AU → `~/Library/Audio/Plug-Ins/Components/`

预编译的 macOS 产物采用 ad-hoc 签名，**未经过 Apple notarization**。如果宿主因下载隔离属性而拒绝扫描插件，请在复制完成后运行：

```bash
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/SfxrVsti.vst3"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/SfxrVsti.component"
```

如果 Standalone 已复制到 `/Applications`，对应命令为：

```bash
xattr -dr com.apple.quarantine "/Applications/SfxrVsti.app"
```

### Windows

将 `.vst3` 复制到 `C:\Program Files\Common Files\VST3\`（需管理员权限）。

### Linux

将 `.vst3` 复制到 `~/.vst3/`。

## 从源码构建

依赖：CMake 3.24+、C++17 编译器。JUCE 8.0.15 通过 CMake `FetchContent` 在配置阶段自动下载（需联网）。

macOS / Linux：

```bash
./scripts/build.sh
```

Windows：

```bat
scripts\build_windows.bat
```

或手动：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 各平台产物

| 平台 | 格式 | 产物位置 |
|------|------|----------|
| macOS | VST3 + AU + Standalone | `build/SfxrVsti_artefacts/Release/` |
| Windows | VST3 + Standalone | `build/SfxrVsti_artefacts/Release/` |
| Linux | VST3 + Standalone | `build/SfxrVsti_artefacts/Release/` |

Windows CI 使用 MSVC；Linux 需 ALSA/JACK/X11 等开发库（见 `.github/workflows/build.yml` 的完整依赖列表）。MinGW 当前未在 CI 中验证。

## 使用

### 界面

- **GENERATOR 列**（左）：7 个预设类别（PICKUP/COIN、LASER/SHOOT、EXPLOSION、POWERUP、HIT/HURT、JUMP、BLIP/SELECT）+ PLAY SOUND / RANDOMIZE / MUTATE / LOAD SOUND / SAVE SOUND
- **MANUAL SETTINGS**（右）：连续参数按 ENVELOPE / FREQUENCY / VIBRATO / SQUARE DUTY / REPEAT / ARPEGGIO / PHASER / FILTERS / VOLUME 分组
- **波形示波器**（底部）：实时显示输出波形
- **虚拟键盘**（底部）：显示 A0–C8 共 88 键，但只有 C2–C6（MIDI 36–84）能通过点击/拖拽触发；范围外按键灰化且不响应；note 69 橙色高亮并标注 ROOT

### 有效音符范围

合成以 note 69 为根音：振荡器周期乘以 `2^(-(note-69)/12)`，因此频率按 `2^((note-69)/12)` 移调。根音本身的频率取决于「Start Frequency」（默认 0.3 时约 321 Hz），所以键盘音名仅用于定位，不代表标准十二平均律音高。

插件的唯一有效触发范围是 **C2–C6（MIDI 36–84，含两端）**。屏幕键盘虽然显示完整 88 键，但范围外按键会灰化且不响应；来自 DAW 的范围外 note-on/note-off 也会被过滤。

这是一个「实用音域」约定而非技术边界：音高是相对 START FREQ 移调的，所以同一个 MIDI 音符的实际频率取决于该参数。默认 0.3 时 C2 约为 48 Hz、C6 约为 764 Hz。

### 参数

| 参数 | 范围 | 说明 |
|------|------|------|
| Waveform | Square/Saw/Sine/Noise | 基础波形 |
| Attack / Sustain / Punch / Decay Time | 0–1 | 音量包络 |
| Start Frequency | 0–1 | 起始频率（note 69 对应此值） |
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

- **采样率适配**：原版常数按 44100 Hz 校准，当前实现按量纲缩放音高、滑音、占空比扫描、滤波器、颤音、包络、repeat、arpeggio 与 phaser 延时。常驻测试直接覆盖音高、包络、颤音、滑音和占空比扫描；滤波器、phaser、repeat、arpeggio 与 vibrato delay 尚缺逐项跨采样率回归测试
- **MIDI 移调**：`fperiod *= 2^(-(note-69)/12)`，note 69 对应原始参数
- **复音状态**：原版的全局变量全部下沉为 voice 实例字段
- **力度**：MIDI velocity 缩放输出增益
- **`vib_delay` 生效**：原版该参数从未使用，现实现为「延迟后淡入」
- **Sustain 模式**：One-Shot 关闭时，音符停在 Sustain 阶段直到 note-off
- **包络除法判零**：原版在阶段长度为 0 时 0/0 得到 NaN，写 WAV 无妨，但送进 DAW 母线不可接受。判零而不是给长度加下限，保证非退化声音与原版逐样本一致（44.1 kHz 下与移植前 97% 逐位相同，最大偏差 8.5e-07，仅来自系数改用 double 计算）

## 测试

`tests/RenderTest.cpp` 是一组带断言的离线测试，不依赖宿主，CI 每次构建都会运行：

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

覆盖内容：

- 音高、包络时长、颤音速率、温和滑音、占空比扫描在 **44.1 / 48 / 88.2 / 96 / 192 kHz** 下的一致性
- MIDI 移调符合十二平均律（±1 八度、+7 半音）
- 遍历 7 类预设生成器与 randomize/mutate 共 692 组参数，检查**单个 voice** 的输出有限且不越过其 ±1 限幅；该测试不覆盖恶意 `.sfs` 中的 NaN/Inf 或多 voice 混音峰值
- 验证被合成器平方使用的越界参数在取 `abs()` 后渲染结果逐位相同，并检查 10850 组生成参数全部落在取值域内
- 输出电平与原版 WAV 导出一致，且随 Output Level 线性变化
- One-Shot 自行结束、Sustain 模式持续到 note-off
- `.sfs` v102 的部分代表字段可往返，截断文件与未知版本号会被拒绝；v100/v101 和全部字段的 golden fixture 尚未加入测试

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
