# Boostor

<p align="center">
  <img src="Assets/gain-knob-v2.png" width="240" alt="Boostor">
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/boostor/actions/workflows/ci.yml"><img src="https://github.com/conjius-dp/boostor/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/conjius-dp/boostor/releases/latest"><img src="https://img.shields.io/github/v/release/conjius-dp/boostor?label=stable" alt="Stable"></a>
  <a href="https://github.com/conjius-dp/boostor/releases"><img src="https://img.shields.io/github/v/release/conjius-dp/boostor?include_prereleases&label=nightly" alt="Nightly"></a>
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/boostor/releases/latest">
    <img src="https://img.shields.io/badge/Download_for_macOS-d48300?style=for-the-badge&logo=apple&logoColor=white" alt="Download for macOS">
  </a>
</p>

A simple audio plugin with a single gain knob ranging from -inf (silence) to +24 dB. The gain parameter is fully automatable, so DAWs can map it to any MIDI controller.

**Formats:** VST3, AU, Standalone
**Platform:** macOS only (for now)

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
