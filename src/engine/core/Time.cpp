#include "Time.h"
#include <chrono>
using namespace std::chrono;
namespace nova {
double Time::s_last = 0; float Time::s_dt = 0.f;
void Time::BeginFrame(){
    double now = duration<double>(steady_clock::now().time_since_epoch()).count();
    if (s_last==0) { s_last = now; s_dt = 0; return; }
    s_dt = float(now - s_last); s_last = now;
}
float Time::Delta(){ return s_dt; }
}
