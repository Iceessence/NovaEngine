#include "engine/editor/Editor.h"

#ifndef NOVA_INFO
#define NOVA_INFO(...) do{}while(0)
#endif

int main()
{
    NOVA_INFO("NovaEditor starting...");

    nova::Editor e;
    e.Init();
    e.Run();
    e.Shutdown();

    NOVA_INFO("Goodbye.");
    return 0;
}
