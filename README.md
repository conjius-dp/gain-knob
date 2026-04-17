# boostor

<p align="center">
  <picture><img src="https://conjius-dp.github.io/boostor/screenshot.png" width="210" alt="Boostor"></picture>
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/boostor/releases/latest/download/Boostor-macOS-VST3.zip"><img src="Assets/badges/download-vst3-macos.png" height="32" alt="Download VST3 for macOS"></a>
  &nbsp;
  <a href="https://github.com/conjius-dp/boostor/releases/latest/download/boostor-macOS-AU.zip"><img src="Assets/badges/download-au-macos.png" height="32" alt="Download AU for macOS"></a>
  &nbsp;
  <a href="https://github.com/conjius-dp/boostor/releases/latest/download/Boostor-Windows-VST3.zip"><img src="Assets/badges/download-vst3-windows.png" height="32" alt="Download VST3 for Windows"></a>
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/boostor/actions/workflows/ci.yml"><img src="https://github.com/conjius-dp/boostor/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/conjius-dp/boostor/releases/latest"><img src="https://img.shields.io/github/v/release/conjius-dp/boostor?label=stable&color=brightgreen" alt="Stable"></a>
  <a href="https://github.com/conjius-dp/boostor/releases"><img src="https://img.shields.io/github/v/release/conjius-dp/boostor?include_prereleases&label=nightly" alt="Nightly"></a>
</p>

A simple audio plugin with a single gain knob ranging from -inf (silence) to +24 dB. The gain parameter is fully automatable, so DAWs can map it to any MIDI controller.

Available as **VST3** (macOS, Windows), **AU** & **Standalone** (macOS) formats with stereo input/output.

## Dependencies

| Dependency | Version | Notes |
|---|---|---|
| [JUCE](https://github.com/juce-framework/JUCE) | **8.0.12** | Must be cloned into the project root as `JUCE/` |
| CMake | **≥ 3.22** | Tested with 3.29.2 |
| C++ compiler | **C++17** | Tested with Apple Clang 17.0.0 |
| [Ninja](https://ninja-build.org/) | **≥ 1.11** | Fast build system, replaces Make |
| [ccache](https://ccache.dev/) | **≥ 4.0** | Compiler cache for faster rebuilds |

## Setup

1. Clone the repository:

```bash
git clone https://github.com/conjius-dp/boostor.git
cd boostor
```

2. Clone JUCE 8.0.12 into the project root:

```bash
git clone --branch 8.0.12 --depth 1 https://github.com/juce-framework/JUCE.git
```

3. Install Ninja and ccache:

```bash
brew install ninja ccache
```

4. Configure and build:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache
cmake --build build
```

Ninja builds all targets in parallel by default. ccache caches compiled objects so incremental rebuilds after small changes are near-instant.

The built plugins are in `build/Boostor_artefacts/Release/`:

- `VST3/boostor.vst3`
- `AU/boostor.component`
- `Standalone/boostor.app`

On macOS, `COPY_PLUGIN_AFTER_BUILD` is enabled so the VST3 and AU are automatically installed to `~/Library/Audio/Plug-Ins/`.

## Running Tests

```bash
cmake --build build --target BoostorTests
./build/BoostorTests_artefacts/Release/BoostorTests
```
