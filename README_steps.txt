This bundle helps fix the 'ui_frame_begun: undeclared identifier' error and ImGui frame asserts
when NOVA_DISABLE_IMGUI is set.

CONTENTS
- scripts\apply_editor_ui_flag.ps1 : PowerShell script to insert a safe declaration
  of 'bool ui_frame_begun = false;' at the start of Editor::Run().
- Editor_ui_guard.patch : A git patch that adds proper ImGui guarding (optional).
- README_steps.txt : These instructions.

QUICK USAGE (PowerShell)
1) Extract this zip into your repo root: C:\Dev\NovaEngine
2) Run the script:
   powershell -ExecutionPolicy Bypass -File scripts\apply_editor_ui_flag.ps1 -RepoRoot .
3) Rebuild and run:
   cmake --build build --config Debug
   .\build\bin\Debug\NovaEditor.exe

OPTIONAL (if you use git): apply the patch to guard ImGui calls
  git apply Editor_ui_guard.patch

If anything looks off, restore the backup created at:
  src\engine\editor\Editor.cpp.bak
