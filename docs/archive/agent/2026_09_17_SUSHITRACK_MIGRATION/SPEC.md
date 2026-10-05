# SushiTrack Migration Specification

## 1. Objective
Rebrand the codebase from SushiTrack to SushiTrack, re-license under Apache License Version 2.0 open-source, and adapt the repository to full Sushi Systems engineering doctrine (`sushiskills`).

## 2. Invariants & Requirements
1. **Clean Break:** Absolutely NO legacy `SushiTrack` backwards-compatibility shims, aliases, or fallbacks.
2. **Copyright:** `Copyright (c) 2026-present Mustafa Garip & Sushi Systems`.
3. **Organization:** `https://sushisystems.io`, GitHub: `https://github.com/SushiSystems/SushiTrack`.
4. **C API Prefix:** Strictly `sushitrack_` (NOT abbreviated `st_`).
5. **CLI:** Must support both `st` and `sushitrack` as invocation commands.
6. **Build / Test Directive (Directive #3):** NEVER invoke `cmake`, `ninja`, `ctest`, or compiler directly. The CLI is the ONLY door: `python cli/st.py build`, `python cli/st.py test`.
7. **C++ Standard (Directive #4):** C++17 only (`-std=c++17`). No C++20 features.
8. **Layout:** Standard single-module layout: `include/SushiTrack/`, `source/`, `third_party/`, `tools/`, `tests/`, `docs/`.
