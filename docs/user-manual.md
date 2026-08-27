# SfxrVsti 用户手册

> [English version](user-manual.en.md)

本手册面向插件使用者，介绍 SfxrVsti 的安装、界面、参数与使用技巧。SfxrVsti 是基于 [sfxr](http://www.drpetter.se/project_sfxr.html) 合成算法移植的乐器插件，用于生成复古电子游戏风格的音效。

---

## 目录

1. [安装](#1-安装)
2. [快速上手](#2-快速上手)
3. [界面总览](#3-界面总览)
4. [参数详解](#4-参数详解)
5. [预设生成器](#5-预设生成器)
6. [虚拟键盘与音符范围](#6-虚拟键盘与音符范围)
7. [波形示波器](#7-波形示波器)
8. [保存与加载 .sfs 文件](#8-保存与加载-sfs-文件)
9. [在 DAW 中使用](#9-在-daw-中使用)
10. [常见问题](#10-常见问题)

---

## 1. 安装

### 下载

从 [GitHub Releases](../../releases) 下载对应平台的预编译产物，解压后安装：

- macOS → `SfxrVsti-macOS.zip`（VST3 + AU + Standalone）
- Windows → `SfxrVsti-Windows.zip`（VST3 + Standalone）
- Linux → `SfxrVsti-Linux.zip`（VST3 + Standalone）

### 系统要求

- macOS / Windows / Linux
- 支持 VST3、AU（仅 macOS）或 Standalone 的宿主

### macOS

将解压出的插件复制到用户插件目录：

| 格式 | 安装位置 |
|------|----------|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| AU | `~/Library/Audio/Plug-Ins/Components/` |

预编译的 macOS 产物采用 ad-hoc 签名，**未经过 Apple notarization**。如果宿主因下载隔离属性而拒绝扫描插件，请在复制完成后运行：

```bash
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/SfxrVsti.vst3"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/SfxrVsti.component"
```

如果 Standalone 已复制到 `/Applications`：

```bash
xattr -dr com.apple.quarantine "/Applications/SfxrVsti.app"
```

安装后重启宿主（或触发音频插件重新扫描）即可在乐器列表中看到「SfxrVsti」。

### Windows

将 `SfxrVsti.vst3` 复制到 `C:\Program Files\Common Files\VST3\`（需管理员权限）。

### Linux

将 `SfxrVsti.vst3` 复制到 `~/.vst3/`。

### Standalone

Standalone 应用无需宿主，双击即可运行，用于快速试听。

---

## 2. 快速上手

1. 在 DAW 中新建一条乐器轨，加载 SfxrVsti。
2. 点击左侧 **GENERATOR** 列的任意预设按钮（如 PICKUP/COIN），立即听到一个随机生成的声音。
3. 用 MIDI 键盘（或底部虚拟键盘）演奏不同音高。
4. 不满意时点击 **MUTATE** 微调，或 **RANDOMIZE** 完全随机。
5. 用右侧滑条精调参数，点 **SAVE SOUND** 保存为 `.sfs` 文件。

---

## 3. 界面总览

插件窗口大小为 880×700，分为几个区域：

```
┌─────────────────────────────────────────────────────────┐
│ 波形选择(SQUARE/SAW/SINE/NOISE)      MONO  ONE-SHOT      │
├──────────┬──────────────────────────────────────────────┤
│ GENERATOR│  MANUAL SETTINGS                             │
│ 预设类别  │  ENVELOPE    FREQUENCY    ARPEGGIO          │
│          │  VIBRATO     SQUARE DUTY  PHASER            │
│ PLAY     │  REPEAT      FILTERS      VOLUME            │
│ RANDOMIZE│                                              │
│ MUTATE   │                                              │
│ LOAD/SAVE│                                              │
├──────────┴──────────────────────────────────────────────┤
│                   波形示波器                             │
├─────────────────────────────────────────────────────────┤
│                   虚拟 MIDI 键盘 (88 键)                 │
└─────────────────────────────────────────────────────────┘
```

- **顶部**：波形类型选择、MONO 与 ONE-SHOT 开关
- **左侧**：预设生成器与文件操作
- **右侧**：全部合成参数（分组排列）
- **底部**：实时波形示波器 + 虚拟 MIDI 键盘

---

## 4. 参数详解

参数分为若干组，多数取值 0–1；带 ± 号的是双极参数，取值 -1–1（滑条中点的黑色刻度表示 0）。

### 4.1 波形（Waveform）

| 选项 | 说明 |
|------|------|
| SQUAREWAVE | 方波，可用 Square Duty 调占空比 |
| SAWTOOTH | 锯齿波，音色明亮 |
| SINEWAVE | 正弦波，最纯净 |
| NOISE | 噪声，适合爆炸/冲击类音效 |

### 4.2 包络（ENVELOPE）

控制音量随时间的变化，决定音效的「起音—保持—衰减」轮廓。

| 参数 | 范围 | 说明 |
|------|------|------|
| ATTACK TIME | 0–1 | 起音时长；越大起始越平滑 |
| SUSTAIN TIME | 0–1 | 音量保持的时长 |
| SUSTAIN PUNCH | 0–1 | 保持阶段初期的「冲击」量，产生弹跳感 |
| DECAY TIME | 0–1 | 衰减（淡出）时长 |

### 4.3 频率（FREQUENCY）

| 参数 | 范围 | 说明 |
|------|------|------|
| START FREQ | 0–1 | 起始频率；根音（note 69）对应此值 |
| MIN FREQ | 0–1 | 频率下限；下滑触底即停止发声 |
| SLIDE | ± | 频率滑移速率（正=上滑，负=下滑） |
| DELTA SLIDE | ± | 滑移速率本身的变化率（滑移的加速/减速） |

### 4.4 颤音（VIBRATO）

| 参数 | 范围 | 说明 |
|------|------|------|
| DEPTH | 0–1 | 颤音深度（音高摆动的幅度） |
| SPEED | 0–1 | 颤音速度 |
| DELAY | 0–1 | 颤音延迟：发声后延迟多久才开始颤音（并逐渐淡入） |

### 4.5 琶音 / 音高突变（ARPEGGIO）

| 参数 | 范围 | 说明 |
|------|------|------|
| CHANGE AMOUNT | ± | 音高突变的方向与幅度（正=上移，负=下移） |
| CHANGE SPEED | 0–1 | 延迟多久后发生突变（越大越早） |

### 4.6 方波占空比（SQUARE DUTY）

| 参数 | 范围 | 说明 |
|------|------|------|
| DUTY | 0–1 | 方波正半周的占空比 |
| DUTY SWEEP | ± | 占空比的扫频速率 |

### 4.7 重复（REPEAT）

| 参数 | 范围 | 说明 |
|------|------|------|
| REPEAT SPEED | 0–1 | 周期性重置频率与占空比（包络与滤波器不受影响），产生脉冲/节奏感；0 关闭 |

### 4.8 相位器（PHASER）

| 参数 | 范围 | 说明 |
|------|------|------|
| OFFSET | ± | 叠加的延迟量，产生紧凑混响/科幻感 |
| SWEEP | ± | 延迟量的扫频速率 |

### 4.9 滤波器（FILTERS）

| 参数 | 范围 | 说明 |
|------|------|------|
| LP CUTOFF | 0–1 | 低通截止频率（1 = 关闭） |
| LP SWEEP | ± | 低通截止频率扫频 |
| LP RESONANCE | 0–1 | 低通谐振（峰值） |
| HP CUTOFF | 0–1 | 高通截止频率（0 = 关闭），可去除低频杂音 |
| HP SWEEP | ± | 高通截止频率扫频 |

### 4.10 输出与模式

| 参数 | 范围 | 说明 |
|------|------|------|
| OUTPUT LEVEL | 0–1 | 输出音量（调太高可能削波） |
| MONO | 开关 | 单音模式：新音符重新触发唯一 voice |
| ONE-SHOT | 开关 | 开启=一次触发（像鼓，播完即止）；关闭=持续模式（按住持续，松开释放） |

---

## 5. 预设生成器

左侧按钮一键生成不同风格的音效（每次点击随机）：

| 按钮 | 典型音色 |
|------|----------|
| PICKUP/COIN | 拾取金币 |
| LASER/SHOOT | 激光/射击 |
| EXPLOSION | 爆炸 |
| POWERUP | 强化道具 |
| HIT/HURT | 受击 |
| JUMP | 跳跃 |
| BLIP/SELECT | 菜单选择提示音 |

其他操作：

- **RANDOMIZE**：完全随机化所有参数
- **MUTATE**：在当前参数基础上做小幅随机扰动
- **PLAY SOUND**：立即以根音（note 69）试听当前声音

---

## 6. 虚拟键盘与音符范围

底部为 88 键全键盘（A0–C8）：

- **白色/黑色**键可点击，按住并拖动可滑奏
- **根音**（note 69）以橙色高亮并标注「ROOT」——它直接播放「START FREQ」旋钮的原值。注意它的实际频率由 START FREQ 决定（默认 0.3 时约 321 Hz），并不是 440 Hz 的标准 A4，所以键盘上的音名只作定位参考
- **C2–C6 之外**的键灰化，不响应鼠标；DAW 传入的对应 MIDI 音符也同样被忽略，两者行为一致

### 为什么限制在 C2–C6？

合成以 note 69 为根音按半音移调，但插件只响应 C2–C6（MIDI 36–84，含两端）。范围外的 note-on/note-off 会被过滤；屏幕键盘虽显示 A0–C8 共 88 键，范围外按键也不会触发声音。

这是一个「实用音域」约定，不是技术边界。音高是在 START FREQ 的基础上按半音移调的，所以同一个 MIDI 音符的实际频率取决于该参数：默认 0.3 时 C2 约 48 Hz、根音约 321 Hz、C6 约 764 Hz。再往下移调基频会低到听不见，再往上噪声/爆炸类音效容易失真，因此固定取 C2–C6 作为默认的可用区间。

如果你把 START FREQ 调得很低或很高，可用音域会整体平移，这时 C2–C6 未必是最合适的窗口。

---

## 7. 波形示波器

位于键盘上方的实时示波器，显示当前输出波形，用于直观判断音效的形状与电平。播放时波形会实时滚动更新。

---

## 8. 保存与加载 .sfs 文件

- **SAVE SOUND**：将当前全部参数保存为 `.sfs` 文件
- **LOAD SOUND**：加载 `.sfs` 文件

`.sfs` 与原版 sfxr 的文件格式字节兼容（版本 102），可与原版 sfxr 或 jsfxr 等工具互用。

> 注意：`.sfs` 仅保存合成参数，不包含 Mono / One-Shot 等插件级设置。

---

## 9. 在 DAW 中使用

### MIDI 演奏

- 直接用 MIDI 键盘/钢琴卷帘演奏，音高按 note 69 根音移调
- 力度（velocity）控制输出音量
- 8 复音，可演奏和弦；打开 MONO 则退化为单音

### 参数自动化

所有合成参数都暴露给宿主，可在自动化轨道中录制/绘制曲线。

注意：与原版 sfxr 一致，参数在 **note-on 时刻锁存**到该音符，之后的自动化不会改变正在发声的音符——效果要到下一个音符才体现。因此自动化 SLIDE、DELTA SLIDE、REPEAT SPEED 等参数配合密集的音符序列，可产生音效逐步「演变」的效果。

### 触发模式建议

- 制作一次性音效（如爆炸、射击）：保持 ONE-SHOT 开启
- 制作可演奏的持续音（如激光、警报）：关闭 ONE-SHOT，配合长 SUSTAIN TIME

---

## 10. 常见问题

**Q：点了虚拟键盘没声音？**
确认 Output Level 不为 0，且按的键在 C2–C6 之内（范围外的键已灰化禁用）。

**Q：点击预设按钮，听到的却是上一个声音？**
预设已更新参数，播放试听使用最新参数。若仍异常请检查宿主是否正常运行音频回调。

**Q：为什么低于 C2 / 高于 C6 的音符不发声？**
有效范围固定为 C2–C6（MIDI 36–84），范围外的 MIDI 事件会被过滤，UI 上也已灰化。这是可用音域约定：音高相对 START FREQ 移调，往下太多基频会低到听不见，往上太多会接近采样率上限而失真。

**Q：加载 `.sfs` 后 Mono/One-Shot 模式没变？**
`.sfs` 不含 Mono、One-Shot 等插件级设置，这些需在界面上单独设置；Output Level 会随 `.sfs` 一并保存/加载。

**Q：输出削波？**
降低 OUTPUT LEVEL，或降低宿主轨道的输入增益。sfxr 的波形经 phaser 叠加后峰值较高。
