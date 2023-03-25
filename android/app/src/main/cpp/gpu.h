#ifndef GPU_H
#define GPU_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

#include "defines.h"

class Core;

class Gpu {
public:
    Gpu(Core *core);

    ~Gpu();

    void scheduleInit();

    void gbaScheduleInit();

    bool getFrame(uint32_t *out, bool gbaCrop);

    void invalidate3D() { dirty3D |= BIT(0); }

    uint16_t readDispStat(bool cpu) { return dispStat[cpu]; }

    uint16_t readVCount() { return vCount; }

    uint32_t readDispCapCnt() { return dispCapCnt; }

    uint16_t readPowCnt1() { return powCnt1; }

    void writeDispStat(bool cpu, uint16_t mask, uint16_t value);

    void writeDispCapCnt(uint32_t mask, uint32_t value);

    void writePowCnt1(uint16_t mask, uint16_t value);

private:
    Core *core;

    struct Buffers {
        uint32_t *framebuffer = nullptr;
        uint32_t *hiRes3D = nullptr;
        bool top3D = false;
    };

    std::queue<Buffers> framebuffers;
    std::atomic<bool> ready;
    std::mutex mutex;

    bool running = false;
    std::atomic<int> drawing;
    std::thread *thread = nullptr;

    bool gbaBlock = true;
    bool displayCapture = false;
    uint8_t dirty3D = 0;

    uint16_t dispStat[2] = {};
    uint16_t vCount = 0;
    uint32_t dispCapCnt = 0;
    uint16_t powCnt1 = 0;

    std::function<void()> gbaScanline240Task;
    std::function<void()> gbaScanline308Task;
    std::function<void()> scanline256Task;
    std::function<void()> scanline355Task;

    static uint32_t rgb5ToRgb8(uint32_t color);

    static uint32_t rgb6ToRgb8(uint32_t color);

    static uint16_t rgb6ToRgb5(uint32_t color);

    void gbaScanline240();

    void gbaScanline308();

    void scanline256();

    void scanline355();

    void drawGbaThreaded();

    void drawThreaded();
};

#endif // GPU_H
