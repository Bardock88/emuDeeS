#ifndef CORE_H
#define CORE_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "bios.h"
#include "cartridge.h"
#include "cp15.h"
#include "defines.h"
#include "div_sqrt.h"
#include "dldi.h"
#include "dma.h"
#include "gpu.h"
#include "gpu_2d.h"
#include "gpu_3d.h"
#include "gpu_3d_renderer.h"
#include "input.h"
#include "interpreter.h"
#include "ipc.h"
#include "memory.h"
#include "rtc.h"
#include "spi.h"
#include "spu.h"
#include "timers.h"
#include "wifi.h"

enum CoreError {
    ERROR_BIOS = 1,
    ERROR_FIRM,
    ERROR_ROM
};

struct Task {
    Task(std::function<void()> *task, uint32_t cycles) : task(task), cycles(cycles) {}

    std::function<void()> *task;
    uint32_t cycles;

    bool operator<(const Task &task) const { return cycles < task.cycles; }
};

class Core {
public:
    Core(const std::string& ndsPath = "", const std::string& gbaPath = "", int id = 0);

    void runFrame() { (this->*runFunc)(); }

    bool isGbaMode() { return gbaMode; }

    bool isRunning() { return running; }

    int getId() { return id; }

    int getFps() { return fps; }

    uint32_t getGlobalCycles() { return globalCycles; }

    void schedule(Task task);

    void enterGbaMode();

    void endFrame();

    Bios9 bios9;
    Bios7 bios7;
    CartridgeNds cartridgeNds;
    CartridgeGba cartridgeGba;
    Cp15 cp15;
    DivSqrt divSqrt;
    Dldi dldi;
    Dma dma[2];
    Gpu gpu;
    Gpu2D gpu2D[2];
    Gpu3D gpu3D;
    Gpu3DRenderer gpu3DRenderer;
    Input input;
    Interpreter interpreter[2];
    Ipc ipc;
    Memory memory;
    Rtc rtc;
    Spi spi;
    Spu spu;
    Timers timers[2];
    Wifi wifi;

private:
    void (Core::*runFunc)() = &Core::runNdsFrame;

    bool gbaMode = false;
    int id = 0;

    std::vector<Task> tasks;
    uint32_t globalCycles = 0;
    uint32_t arm9Cycles = 0, arm7Cycles = 0;

    std::atomic<bool> running;
    int fps = 0, fpsCount = 0;
    std::chrono::steady_clock::time_point lastFpsTime;

    std::function<void()> resetCyclesTask;

    void resetCycles();

    void runNdsFrame();

    void runGbaFrame();
};

#endif // CORE_H
