# Safety review / 安全性確認 — v0.17.0

Review date / 確認日: 2026-10-26

## Scope / 対象

The public source tree and the rebuilt ARM unit were reviewed. The rebuilt binary matches the previously delivered v0.17.0 byte-for-byte. This report describes checks performed; it is not a guarantee of hardware behavior.

公開用ソースと再ビルドしたARMユニットを確認しました。既存v0.17.0とバイト単位で一致しています。実施した検査の記録であり、実機動作の保証ではありません。

| Check / 検査 | Result / 結果 |
|---|---|
| Native API suite with AddressSanitizer and UndefinedBehaviorSanitizer | PASS; no sanitizer findings |
| 3,000 adversarial parameter/event cases, including INT_MIN/INT_MAX and invalid IDs | PASS; finite output, output bounds, intact guard buffers |
| Actual ARM shared unit loaded under QEMU | PASS |
| All 58 words, one-shot gates and callback block invariance | PASS |
| All 5,858 numeric INDEX positions audible and within trimmed ranges | PASS |
| 522 stutter conditions, loop bounds and short-gate completion | PASS |
| Sixteenth-note rest at BPM 60/120/240 | PASS |
| 76 PITCH display keys, random-control independence and extreme controls | PASS |
| Stereo echo pairs, feedback, sync divisions, reset, suspend and output bounds | PASS |
| Removed output PAN ignores stale parameter writes | PASS |
| Source audio, model weights, credentials and local private paths in publish allowlist | None found |
| Release ZIP audio-file and secret scan | PASS |

## Code review / コード確認

The runtime unit does not read the audio input, open files, use network APIs, launch processes, or load models. Its imported functions are math, formatting, memory initialization and standard ELF runtime helpers. Audio-thread code uses fixed storage and atomic control values; there are no per-render heap allocations. The delay ring is bounded to 524,288 floats, with maximum four-second single-echo interval and feedback capped at 75%. Software output bounds were checked on both stereo channels. Static BSS is 2,097,440 bytes; this is not a measurement of total process memory or hardware CPU load.

ユニット実行時に入力音声の取得、ファイル読書き、通信、外部プロセス起動、モデル読み込みを行う処理はありません。音声処理内でヒープ確保を行わず、固定長領域とatomicで制御します。ディレイは524,288個のfloat、単一間隔は最大4秒、FBは75%まで。両チャンネルの出力上限を検査しました。BSSは2,097,440バイトですが、総メモリ使用量・実機CPU負荷の実測値ではありません。

## Publication boundary / 公開範囲

Source, documentation and licenses are committed in a fresh repository history. The release adds the unit binary and checksums. Audio files (including rendered previews and raw float captures), models, previous work archives, personal environment scripts, tokens, source paths and recorded RU VOC1/RU VOC2 are excluded. An explicit path allowlist is used for both Git staging and release packaging. The parameter bank still intentionally contains enough information to resynthesize the published phrases.

新規Git履歴へ公開対象だけを追加します。元WAV、試聴、float生音声、モデル、過去の作業ZIP、個人環境操作スクリプト、認証情報、原音の保存パス、ユーザー録音RU VOC1/RU VOC2は含みません。Git登録・ZIP作成の両方に明示的なファイル許可リストを使用します。LPC係数とフレーズ本文は音源機能のために公開します。

## Build and license review / ビルドとライセンス

The KORG SDK-derived notices and BSD-3-Clause license are retained. Original project material uses MIT, matching the owner's previous drumlogue publication. Upstream VoxCPM2 is attributed; no model weights or code are bundled. See THIRD_PARTY_NOTICES.md.

The SDK build image is pinned to `xiashj/logue-sdk@sha256:e4d85a16c38dc4d34b0e93cabb6378df21729688dbcd88808c84e8d41aa0c9ba`. Native ASan/UBSan tests and QEMU ran in the local SDK test image (g++ and qemu-user added). Sanitizers cover native execution, not the deployed ARM binary; that binary was tested separately under QEMU. Build scripts only mount this repository and build the unit. They do not change Docker settings or stop system services.

KORG由来の著作権表示とBSD-3-Clauseを保持し、独自部分は以前の公開方針に合わせてMITとしています。VoxCPM2の出典を記載し、モデルは同梱しません。ビルドコンテナはdigest固定。Sanitizerはネイティブ実行が対象で、配布ARMバイナリは別途QEMUで検査しています。公開ビルドスクリプトはDockerの設定変更やサービス停止を行いません。

## Limits / 未確認項目

Hardware CPU load, subjective sound quality, display behavior, MOTION recording/playback and external tempo synchronization have not been measured or verified. No timing guarantee is implied by QEMU. The private developer/unit IDs are documented and should be changed by downstream projects that fork the unit. Start playback at a low monitoring level, particularly with the +18 dB internal speech gain. If a problem occurs, remove the unit and restart the device.

実機CPU負荷、主観的音質、表示、MOTION記録・再生、外部同期は未確認です。QEMUでリアルタイム性能を保証するものではありません。派生版を配布する場合は独自IDへ変更してください。＋18dB内部ゲインがあるため低いモニター音量から確認し、異常が続く場合はユニットを外して本体を再起動してください。
