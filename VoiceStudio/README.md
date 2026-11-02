# Voice Studio

Local-first VST3 voice processing prototype for Windows and Wave Link.

## Safety contract

- Audio processing only: the plugin receives and outputs audio through the host's normal VST3 interface.
- No game process access, memory reads/writes, DLL injection, hooks, anti-cheat bypasses, kernel drivers, or stealth behavior.
- No telemetry, network calls, account sign-in, or cloud inference in the plugin.
- AI inference is opt-in and disabled by default.
- The non-AI DSP path must remain fully functional when the AI engine is absent or stopped.
- The AI worker, if introduced, must be a visible user-controlled process with explicit start/stop status.
- Never attempt to disable or evade FACEIT Anti-Cheat, Easy Anti-Cheat, or other security software.

## Planned architecture

1. **VST3 host plugin** — JUCE-based audio input/output, parameter state, DSP effects, UI.
2. **Optional local inference worker** — isolated process for a vetted RVC-compatible engine. It is not included until its license, dependencies, model format, and IPC design are reviewed.
3. **Audio IPC** — bounded, local-only transport with explicit lifecycle, timeouts, and fail-safe dry/bypass behavior.
4. **Model assets** — separately supplied by the user only after provenance, licensing, and integrity checks are documented.

## Initial voice presets

- Radio Operator
- Deep Male
- Female Voice
- Robot
- Demon
- Anime Voice

Preset names are targets for configuration slots, not bundled voice models.

## Resource lifecycle

- AI is off by default.
- Turning AI off starts a 30-second warm retention period.
- After the retention period, the worker releases the model and GPU resources.
- **UNLOAD AI NOW** requests immediate worker shutdown and resource release.
- If the worker fails, times out, or is unavailable, audio falls back to the non-AI DSP path.

## Current status

Prototype planning and repository isolation only. No AI engine, voice models, external binaries, drivers, or background services are bundled.
