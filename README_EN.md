# LPC ROBOT 7

[日本語](README.md) · [Download](https://github.com/ufotone/LPC-ROBOT-7/releases/latest) · [Word list](WORDS.md)

An LPC speech synthesizer for KORG drumlogue. It resynthesizes 58 words and phrases as robotic speech using coarse LPC parameters. Languages: English, Japanese, Chinese, German, French, Italian and Russian.

Introducing **LPC ROBOT 7 v0.17.0**. The unit appears on the device as **LPC ROBOT 7**.

## Installation

1. Download and extract the public ZIP from [Releases](https://github.com/ufotone/LPC-ROBOT-7/releases/latest).
2. Turn off drumlogue, then hold REC while turning it on to enter USB mass-storage mode.
3. Copy `lpc_robot7.drmlgunit` to `Units/Synths/` on the device, replacing an existing file with that name if necessary.
4. Safely eject the device and press PLAY/STOP to leave USB mode.
5. Select `LPC ROBOT 7` as the USER synth on the MULTI track.

Start with a low LEVEL. Internal speech gain is +18 dB, followed by soft peak control and the user LEVEL setting.

## Controls

Choose WORD and trigger it from the sequencer or MIDI. Normal playback is one-shot: a short gate does not cut off the word. A WORD change takes effect on the next trigger.

| Page | Parameter | Range | Default | Function |
|---|---|---|---|---|
| 1 | WORD | 0–57 | 0 | [58-word bank](WORDS.md) |
| 1 | PITCH | −24 to +50 semitones, RND | 0 | Pitch independent of playback speed |
| 1 | SPEED | 25–250%, RND | 100% | LPC parameter traversal speed |
| 1 | INDEX | OFF, 0–100%, RND | OFF | One-shot / freeze / stutter |
| 2 | LEVEL | 0–100% | 80% | Final level, including delay tails |
| 2 | PAN DELAY | 0–100% | 0% | Stereo delay amount |
| 2 | DLY TIME | 40–750 ms | 250 ms | Delay interval when SYNC is OFF |
| 2 | DLY FB | 0–75% | 35% | Delay feedback |
| 3 | DLY SYNC | OFF / ON | OFF | Follow the device tempo |
| 3 | DLY DIV | Ten divisions from 1/32 to 1/2 | 1/8 | Synced delay division |
| 3 | STTR TIME | 20–250 ms | 80 ms | Source segment length for INDEX RND |

PITCH RND and SPEED RND choose independent values per trigger and hold them for that voice. MIDI note 60 is the reference; pitch bend spans ±2 semitones.

### INDEX and stutter

- **OFF:** one-shot playback.
- **0–100%:** freeze an audible LPC frame within the edited playback range. Silence is skipped; the percentage is not an absolute position in the original WAV. A matching gate/note-off releases the voice.
- **RND:** choose an audible random start per trigger, traverse a short segment, and repeat that segment while gated. Each repetition is separated by a **tempo-synced sixteenth-note rest**: 125 ms at BPM 120.

STTR TIME describes the source segment length at SPEED=100%. At 50%, playback takes twice as long. Segments near the end are shortened. Rest length is independent of SPEED and follows tempo even when DLY SYNC is OFF. Every trigger selects a new start; individual loops keep their selected start.

In RND mode, gate-off finishes the current segment before releasing. Even an extremely short gate plays one segment. Without a gate/note-off, looping continues. Delay tails may remain audible during rests or after release. SPEED does not advance the playback position during numeric INDEX freeze.

### Stereo delay and panning

The first echo goes left and the next goes right at equal level. Subsequent pairs decay according to feedback. High PAN DELAY settings reduce the centered dry voice slightly. Use headphones or both MAIN OUTPUT L/R connections to hear the stereo image.

DLY DIV offers `1/32, 1/16T, 1/16, 1/8T, 1/16D, 1/8, 1/8D, 1/4, 1/4D, 1/2`. Intervals are capped at four seconds; exceeding that limit displays `MAX 4s`. Tempo defaults to BPM 120 until the device supplies an update.

Use drumlogue's MIX ROUTE controls for output placement. PAN DELAY controls the delay amount.

## Distribution and licensing

Public files contain source, LPC coefficients, the unit binary and documentation. **No source WAVs, recordings, preview audio, TTS models or personal environment-management scripts are included.** The LPC parameters needed to resynthesize speech and the phrase text are intentionally public.

No TI speech ROMs or game audio recordings are included. This is not a strict TMS5100/5220 emulator.

Original portions use the [MIT License](LICENSE); KORG SDK-derived portions retain [BSD-3-Clause](LICENSE-KORG). See [third-party notices and speech provenance](THIRD_PARTY_NOTICES.md). This is an independent project, not an official KORG product.

## Build and validation

With Docker running, use `./build.ps1` on Windows or `sh build.sh` on Linux/macOS. Output: `dist/lpc_robot7.drmlgunit`. The SDK container is pinned by digest. Rebuilding requires neither the TTS model nor the source WAVs.

On native Linux, run `sh tests/run.sh`; use `SANITIZE=1 sh tests/run.sh` for AddressSanitizer and UndefinedBehaviorSanitizer. Temporary test files are written to `.test-output/`.

Tested on drumlogue hardware. Unit API 1.0.0 is required.

The unit uses private developer ID `0x5534544E` and unit ID `0x4C504337`.
