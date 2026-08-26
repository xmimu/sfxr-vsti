# SfxrVsti User Manual

> [中文版](user-manual.md)

This manual is aimed at plugin users, covering installation, the interface, parameters, and usage tips. SfxrVsti is an instrument plugin ported from the [sfxr](http://www.drpetter.se/project_sfxr.html) synthesis algorithm, for generating retro video-game-style sound effects.

---

## Contents

1. [Installation](#1-installation)
2. [Quick start](#2-quick-start)
3. [Interface overview](#3-interface-overview)
4. [Parameter reference](#4-parameter-reference)
5. [Preset generators](#5-preset-generators)
6. [Keyboard and note range](#6-keyboard-and-note-range)
7. [Waveform oscilloscope](#7-waveform-oscilloscope)
8. [Saving and loading .sfs files](#8-saving-and-loading-sfs-files)
9. [Using in a DAW](#9-using-in-a-daw)
10. [FAQ](#10-faq)

---

## 1. Installation

### Requirements

- macOS / Windows / Linux
- A host supporting VST3, AU (macOS only) or Standalone

### macOS

The `build.sh` script installs the plugins automatically into the user plugin folders:

| Format | Location |
|--------|----------|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| AU | `~/Library/Audio/Plug-Ins/Components/` |

Restart your host (or trigger a plugin rescan) and "SfxrVsti" will appear in the instrument list.

### Windows

Copy `SfxrVsti.vst3` to `C:\Program Files\Common Files\VST3\`.

### Linux

Copy `SfxrVsti.vst3` to `~/.vst3/`.

### Standalone

The Standalone app needs no host — double-click to run it for quick auditioning.

---

## 2. Quick start

1. Create an instrument track in your DAW and load SfxrVsti.
2. Click any preset button in the **GENERATOR** column (e.g. PICKUP/COIN) to hear a randomly generated sound.
3. Play different pitches on a MIDI keyboard (or the on-screen keyboard at the bottom).
4. Not happy? Click **MUTATE** for a small variation, or **RANDOMIZE** for a fully random sound.
5. Fine-tune with the sliders on the right, then click **SAVE SOUND** to save a `.sfs` file.

---

## 3. Interface overview

The window is 880×700 and is divided into several regions:

```
┌─────────────────────────────────────────────────────────┐
│ Waveform (SQUARE/SAW/SINE/NOISE)      MONO  ONE-SHOT    │
├──────────┬──────────────────────────────────────────────┤
│ GENERATOR│  MANUAL SETTINGS                             │
│ presets  │  ENVELOPE    FREQUENCY    ARPEGGIO          │
│          │  VIBRATO     SQUARE DUTY  PHASER            │
│ PLAY     │  REPEAT      FILTERS      VOLUME            │
│ RANDOMIZE│                                              │
│ MUTATE   │                                              │
│ LOAD/SAVE│                                              │
├──────────┴──────────────────────────────────────────────┤
│                   waveform oscilloscope                 │
├─────────────────────────────────────────────────────────┤
│                   on-screen keyboard (88 keys)          │
└─────────────────────────────────────────────────────────┘
```

- **Top**: waveform selector and the MONO / ONE-SHOT toggles
- **Left**: preset generators and file operations
- **Right**: all synthesis parameters (grouped)
- **Bottom**: live waveform oscilloscope + on-screen MIDI keyboard

---

## 4. Parameter reference

Parameters are grouped; most take 0–1, and those marked ± are bipolar (-1–1), with the black tick at the centre of the slider representing 0.

### 4.1 Waveform

| Option | Description |
|--------|-------------|
| SQUAREWAVE | square wave; shape the duty cycle with Square Duty |
| SAWTOOTH | sawtooth; bright timbre |
| SINEWAVE | sine; purest |
| NOISE | noise; good for explosions/impacts |

### 4.2 Envelope (ENVELOPE)

Controls how loudness evolves over time, shaping the attack–sustain–decay contour.

| Parameter | Range | Description |
|-----------|-------|-------------|
| ATTACK TIME | 0–1 | attack length; larger = smoother onset |
| SUSTAIN TIME | 0–1 | how long the volume is held |
| SUSTAIN PUNCH | 0–1 | "punch" at the start of the sustain phase, for a popping effect |
| DECAY TIME | 0–1 | fade-out length |

### 4.3 Frequency (FREQUENCY)

| Parameter | Range | Description |
|-----------|-------|-------------|
| START FREQ | 0–1 | start pitch; A4 (note 69) maps to this value |
| MIN FREQ | 0–1 | lower pitch limit; the sound stops when a downward slide reaches it |
| SLIDE | ± | pitch slide rate (positive = up, negative = down) |
| DELTA SLIDE | ± | rate of change of the slide (slide acceleration) |

### 4.4 Vibrato (VIBRATO)

| Parameter | Range | Description |
|-----------|-------|-------------|
| DEPTH | 0–1 | vibrato depth (pitch wobble amount) |
| SPEED | 0–1 | vibrato speed |
| DELAY | 0–1 | delay before vibrato starts (then fades in) |

### 4.5 Arpeggio / pitch jump (ARPEGGIO)

| Parameter | Range | Description |
|-----------|-------|-------------|
| CHANGE AMOUNT | ± | direction and amount of the pitch jump (positive = up, negative = down) |
| CHANGE SPEED | 0–1 | how long before the jump happens (larger = sooner) |

### 4.6 Square duty (SQUARE DUTY)

| Parameter | Range | Description |
|-----------|-------|-------------|
| DUTY | 0–1 | duty cycle of the square wave's positive half |
| DUTY SWEEP | ± | sweep rate of the duty cycle |

### 4.7 Repeat (REPEAT)

| Parameter | Range | Description |
|-----------|-------|-------------|
| REPEAT SPEED | 0–1 | periodically resets frequency and duty (envelope and filters are unaffected), for pulsing/rhythmic effects; 0 disables it |

### 4.8 Phaser (PHASER)

| Parameter | Range | Description |
|-----------|-------|-------------|
| OFFSET | ± | delay amount of the overlay, for a tight reverb / sci-fi effect |
| SWEEP | ± | sweep rate of the delay amount |

### 4.9 Filters (FILTERS)

| Parameter | Range | Description |
|-----------|-------|-------------|
| LP CUTOFF | 0–1 | low-pass cutoff frequency (1 = off) |
| LP SWEEP | ± | low-pass cutoff sweep |
| LP RESONANCE | 0–1 | low-pass resonance (peak) |
| HP CUTOFF | 0–1 | high-pass cutoff frequency (0 = off); removes low-frequency hum |
| HP SWEEP | ± | high-pass cutoff sweep |

### 4.10 Output and modes

| Parameter | Range | Description |
|-----------|-------|-------------|
| OUTPUT LEVEL | 0–1 | output volume (too high may clip) |
| MONO | toggle | mono mode: a new note re-triggers the single voice |
| ONE-SHOT | toggle | on = one-shot (plays to completion like a drum); off = sustain (holds until released) |

---

## 5. Preset generators

The buttons on the left generate different styles of sound effects in one click (random each time):

| Button | Typical sound |
|--------|---------------|
| PICKUP/COIN | coin pickup |
| LASER/SHOOT | laser / shooting |
| EXPLOSION | explosion |
| POWERUP | power-up |
| HIT/HURT | being hit |
| JUMP | jump |
| BLIP/SELECT | menu selection blip |

Other actions:

- **RANDOMIZE**: fully randomize all parameters
- **MUTATE**: apply small random perturbations to the current parameters
- **PLAY SOUND**: immediately audition the current sound at A4

---

## 6. Keyboard and note range

The bottom shows the full 88-key keyboard (A0–C8):

- **White/black** keys are clickable; press and drag to glissando
- **A4** is highlighted in orange with an "A4" label — it is the reference pitch for the START FREQ knob
- Keys outside **C2–C6** are greyed out, ignore mouse clicks, and corresponding DAW MIDI notes are dropped

### Why C2–C6?

The synth is rooted at A4 and transposes in semitones. Below C2 (~6 Hz) is essentially inaudible, and above C6 noise/explosion-type sounds tend to distort, so the valid trigger range is C2–C6 (MIDI 36–84).

---

## 7. Waveform oscilloscope

The live oscilloscope above the keyboard shows the current output waveform, making it easy to judge a sound's shape and level. The waveform scrolls in real time while playing.

---

## 8. Saving and loading .sfs files

- **SAVE SOUND**: save all current parameters to a `.sfs` file
- **LOAD SOUND**: load a `.sfs` file

`.sfs` is byte-compatible with the original sfxr file format (version 102), so files can be exchanged with the original sfxr or jsfxr.

> Note: `.sfs` only stores synthesis parameters, not plugin-level settings such as Mono / One-Shot.

---

## 9. Using in a DAW

### MIDI performance

- Play from a MIDI keyboard / piano roll; pitch transposes around the A4 root
- Velocity controls the output level
- 8-voice polyphony supports chords; enable MONO to collapse to a single voice

### Parameter automation

All synthesis parameters are exposed to the host and can be recorded/drawn on automation tracks. Automating SLIDE, DELTA SLIDE or REPEAT SPEED in real time is especially fun for evolving effects.

### Trigger-mode tips

- One-shot effects (explosions, shots): keep ONE-SHOT on
- Playable sustained sounds (lasers, alarms): turn ONE-SHOT off and use a long SUSTAIN TIME

---

## 10. FAQ

**Q: The on-screen keyboard makes no sound?**
Make sure Output Level is non-zero and the note is within the C2–C6 valid range (keys outside it are greyed out and disabled).

**Q: Clicking a preset plays the previous sound?**
The preset updates the parameters and the preview uses the latest values. If it still misbehaves, check that the host is running its audio callback normally.

**Q: Why don't some notes (below C2 / above C6) sound?**
This is by design — the valid note range is fixed at C2–C6, and MIDI events outside it are filtered.

**Q: After loading a .sfs the Mono/One-Shot mode didn't change?**
`.sfs` doesn't contain plugin-level settings such as Mono or One-Shot; set those separately in the UI. Output Level is saved/loaded along with the `.sfs` file.

**Q: The output clips?**
Lower OUTPUT LEVEL or the track's input gain. sfxr's waveform can peak quite high after the phaser overlay.
