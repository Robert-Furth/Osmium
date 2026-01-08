# ![Osmium](/doc/logotype.png)

An oscilliscope visualizer for MIDIs inspired by [corrscope](https://github.com/corrscope/corrscope).

![](/doc/rendering.png)

## Download

Download the executable for your system in the [Releases](https://github.com/Robert-Furth/Osmium/releases) page.
You'll also need to download [FFmpeg](https://ffmpeg.org/download.html) if you don't have it already.

## First-Time Setup

1. Launch Osmium and open the Program Options menu.
2. If FFmpeg isn't in your system path, point Osmium to the place you've installed it.
3. Choose a SoundFont to render your visualizations with.
   - Any will do, so long as it's General MIDI compatible.
     [Here's](https://musical-artifacts.com/artifacts/400) one to get you started.
4. You're all set! Find some MIDIs and get visualizin'!

## Roadmap

This is a project I'm building in my spare time, so progress might be slow.
Nevertheless, here are some things I'd like to add in the future:

- [ ] Saving/loading presets
- [ ] More rendering options (codec, CRF, etc.)
- [ ] Per-track as well as per-channel visualizations
- [ ] MOD music support
- [ ] More control over scope placement
- [ ] Rendering optimizations
- [ ] Waveform stability improvements

## Compiling

You'll need [CMake](https://cmake.org) and a C++20-compatible compiler.

In addition, Osmium relies on the following dependencies:

- [Qt 6](https://www.qt.io/download-dev)
- [BASS and BASSMIDI](https://www.un4seen.com/bass.html) version 2.4.\*
- [Toml++](https://marzer.github.io/tomlplusplus/) version 3.\* (header-only version)

Download those and add them to your include and link paths, then build the project with CMake.
