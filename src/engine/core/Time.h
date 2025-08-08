#pragma once
#include <cstdint>
namespace nova {
struct Time {
    static void BeginFrame();
    static float Delta();
private:
    static double s_last;
    static float s_dt;
};
}
