#include "engine/editor/Editor.h"
#include "engine/core/Log.h"
#include <iostream>

int main() {
    try {
        nova::Log::Init();
        NOVA_INFO("NovaEditor starting...");
        nova::Editor editor;
        editor.Init();
        editor.Run();
        NOVA_INFO("Main: About to call editor.Shutdown()");
        editor.Shutdown();
        NOVA_INFO("Main: editor.Shutdown() completed");
        NOVA_INFO("Goodbye.");
        return 0;
    } catch (const std::exception& e) {
        nova::Log::Init();
        NOVA_FATAL(std::string("Fatal: ") + e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        nova::Log::Init();
        NOVA_FATAL("Unknown fatal error occurred");
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 3;
    }
}

