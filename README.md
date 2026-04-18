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
  <a href="https://github.com/conjius-dp/boostor/actions/workflows/ci.yml"><img src="https://img.shields.io/endpoint?url=https%3A%2F%2Fconjius-dp.github.io%2Fboostor%2Fcoverage.json" alt="Coverage"></a>
  <a href="https://github.com/conjius-dp/boostor/releases/latest"><img src="https://img.shields.io/github/v/release/conjius-dp/boostor?label=stable&color=brightgreen" alt="Stable"></a>
  <a href="https://github.com/conjius-dp/boostor/releases"><img src="https://img.shields.io/github/v/release/conjius-dp/boostor?include_prereleases&label=nightly" alt="Nightly"></a>
  <a href="https://github.com/conjius-dp/boostor/releases"><img src="https://img.shields.io/github/downloads/conjius-dp/boostor/total?label=downloads&color=blue" alt="Total downloads"></a>
</p>

<p align="center">
  <a href="https://github.com/conjius-dp/boostor/graphs/commit-activity"><img src="https://repobeats.axiom.co/api/embed/7d2bd53c5f8ebe8ad748d1e545bdd174a61521ea.svg" width="700" alt="Repobeats analytics image"></a>
</p>

Single-knob gain. −∞ dB to +24 dB, fully automatable, zero algorithmic latency. VST3 (macOS, Windows), AU + Standalone (macOS). Stereo in/out.

## Parameters

| Parameter | Range | Default |
|---|---|---|
| Gain   | −∞ dB – +24 dB | 0 dB |
| Bypass | on / off | off |

Power button (top-right) hard-bypasses the plugin. Input passes through untouched.

## Build

```bash
git clone https://github.com/conjius-dp/boostor.git
cd boostor
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache
cmake --build build
```

VST3 / AU bundles are auto-copied to `~/Library/Audio/Plug-Ins/`.

Requires JUCE 8.0.12, CMake ≥ 3.22, a C++17 compiler, Ninja, ccache.

## Tests

```bash
cmake --build build --target BoostorTests && ./build/BoostorTests_artefacts/Release/BoostorTests
```
