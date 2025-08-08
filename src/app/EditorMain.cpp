#include "engine/editor/Editor.h"
#include "engine/core/Log.h"

int main() {
    try {
    nova::Log::Init();
    NOVA_INFO("NovaEditor starting...");
    nova::Editor editor;
    editor.Init();
    editor.Run();
    editor.Shutdown();
    NOVA_INFO("Goodbye.");
    return 0;
    } catch (const std::exception& e) {
        nova::Log::Init();
        NOVA_FATAL(std::string("Fatal: ") + e.what());
        return 2;
    }
}

