# Changelog

## v0.2.0 (2026-01-08)

- Added configurable templates for channel labels.
  - Channel labels can now include both the instrument name and the channel number using `%i` (instrument name) and `%n` (channel number) escape sequences.
  - Minor text tweaks (e.g. "instrument label" &rarr; "channel label").
- Added two new options for controlling scope behavior: Drift Window and No-Drift Bias.
  - Drift Window specifies how far (in milliseconds) the center of the waveform can be from an x-intercept.
    (Previously, the waveform was always centered exactly on an x-intercept.)
  - No-Drift Bias specifies how much the scope wants to keep the waveform centered on an x-intercept.
    Higher values = less drift.
  - These options should help reduce jitter when rendering complex waveforms (e.g. chords).
- In the program config dialog, added options to control the video codec, encoding speed, and CRF, as well as the audio bitrate.
- Removed the "debug visualization" checkbox.

## v0.1.0 (2025-11-23)

Initial release.
