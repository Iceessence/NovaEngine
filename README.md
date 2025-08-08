# NovaEngine — Best Fix Bundle (CMake include + ImGui env clean)

This bundle applies the **clean** fix so your code can keep `#include "core/Log.h"`:
we add `src/engine` to the include paths for the `NovaEngine` target in **CMakeLists.txt**,
and we clear any `NOVA_DISABLE_IMGUI` environment variable lingering in your shell.

**What it does**
- Appends this (idempotently) to your root `CMakeLists.txt` if missing:
  ```cmake
  target_include_directories(NovaEngine
      PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
  )
  ```
- Clears `NOVA_DISABLE_IMGUI` in the current PowerShell session (does **not** touch user/system env).

**How to use**
1. Unzip this bundle into your repo root (where your `CMakeLists.txt` is).
2. Run (PowerShell):
   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\apply_best_fix.ps1 -RepoRoot .
   ```
3. Re-configure + build (Visual Studio 2022, x64):
   ```powershell
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Debug
   ```
4. Run:
   ```powershell
   .\build\bin\Debug\NovaEditor.exe
   ```

**If you ever want to revert the change**
```powershell
powershell -ExecutionPolicy Bypass -File scripts\revert_best_fix.ps1 -RepoRoot .
```

---
Generated: 2025-08-08 09:09:38
