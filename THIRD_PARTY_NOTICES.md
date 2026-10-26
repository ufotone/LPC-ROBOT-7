# Third-party notices

## KORG logue SDK

`common/`, `lpc-robot/Makefile`, `config.mk`, and the unit interface/template portions are based on the [KORG logue SDK](https://github.com/korginc/logue-sdk) at commit `e78b16c6068b1c30fc072f2aebb8da5eef6c005c`. Copyright notices are retained; see [LICENSE-KORG](LICENSE-KORG) for BSD-3-Clause terms. The original DSP implementation, documentation, tests and generated LPC coefficient bank are provided under [LICENSE](LICENSE).

## Speech provenance

The 58 included voices originated as AI-generated speech made with [OpenBMB VoxCPM2](https://huggingface.co/openbmb/VoxCPM2), followed by editing and LPC analysis. The upstream model card declares Apache-2.0. Model weights, model code, original recordings, source WAVs and preview audio are not distributed here. Speech is represented by reflection coefficients, frame RMS values and voiced/unvoiced flags in `lpc_bank.h`; it is resynthesized by the unit. This is not an attempt to hide the audible content: the synthesized voices and phrase text are intentionally public.

The earlier user-recorded RU VOC1 / RU VOC2 entries are not part of this 58-word bank. No TI speech ROMs, proprietary chip coefficient tables, game sound recordings, or Speak & Spell samples are included. This is an original LPC-inspired synthesizer, not a cycle-accurate TI emulator.

KORG, drumlogue, and the referenced TI product/chip names belong to their respective owners. This project is independent and is not endorsed by KORG, Texas Instruments, or OpenBMB.
