# C_ARDUINO

Small Arduino-specific helpers shared across the author's ESP8266 / ESP32
projects. Sibling of [`C_General`](../C_General) and [`C_ESP`](../C_ESP);
normally vendored under a project's `src/`. Required whenever `C_ESP`'s
`StaticWebServer.hpp` / `StaticWiFi_Conn.hpp` are used, since those
`#include "C_ARDUINO/General.h"`.

## Contents

| File | What it provides |
|------|------------------|
| `General.h` | `avp::urldecode`, `avp::urlencode`, `avp::String_printf`, `avp::TogglePin`, and the `PAUSE_INTERRUPTS` helper. |
| `common.cpp` | Out-of-line implementations; add `+<C_ARDUINO/common.cpp>` to the consuming project's `build_src_filter`. |
| `SDP8xx.hpp` | `avp::SDP8xx` — Sensirion SDP800/SDP810 differential-pressure sensor over `Wire` (header-only, static, no heap). Needs `avp::Crc8` from `C_General`. |

## Usage

Register as a submodule alongside the other two and ensure `src/` is on the
include path so `#include "C_ARDUINO/General.h"` resolves. Older projects that
predate this library often lack the `.gitmodules` entry — adding it is part of
modernizing such a project.

## Repo notes

Canonical: `github.com/avpanasyuk/C_ARDUINO`. `HOME` mirror:
`ssh://BSD/~panasyuk/GIT_REPS/LIBS/C/ARDUINO.git` (bare).
