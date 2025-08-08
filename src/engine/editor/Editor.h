#pragma once

namespace nova {

class Editor {
public:
    void Init();
    void Run();
    void Shutdown();
    Editor() = default;
    void DrawUI();
};

} // namespace nova

