#include <algorithm>
#include <cstring>
#include <thread>

#include "core.h"
#include "settings.h"

Core::Core(const std::string& ndsPath, const std::string& gbaPath, int id) :
        id(id),
        bios9(this),
        bios7(this),
        cartridgeNds(this),
        cartridgeGba(this),
        cp15(this),
        divSqrt(this),
        dldi(this),
        dma{Dma(this, false), Dma(this, true)},
        gpu(this),
        gpu2D{Gpu2D(this, false), Gpu2D(this, true)},
        gpu3D(this),
        gpu3DRenderer(this),
        input(this),
        interpreter{Interpreter(this, false), Interpreter(this, 1)},
        ipc(this),
        memory(this),
        rtc(this),
        spi(this),
        spu(this),
        timers{Timers(this, false), Timers(this, true)},
        wifi(this) {
    // Try to load the ARM9 BIOS; require it when not direct booting
    if (!memory.loadBios9() && (!Settings::getDirectBoot() || (ndsPath.empty() && gbaPath.empty())))
        throw ERROR_BIOS;

    // Try to load the ARM7 BIOS; require it when not direct booting
    if (!memory.loadBios7() && (!Settings::getDirectBoot() || (ndsPath.empty() && gbaPath.empty())))
        throw ERROR_BIOS;

    // Try to load the firmware; require it when not direct booting
    if (!spi.loadFirmware() && (!Settings::getDirectBoot() || (ndsPath.empty() && gbaPath.empty())))
        throw ERROR_FIRM;

    // Try to load the GBA BIOS; require it when booting a GBA ROM
    if (!memory.loadGbaBios() && (!gbaPath.empty() && (ndsPath.empty() || !Settings::getDirectBoot())))
        throw ERROR_BIOS;

    // Schedule initial tasks for NDS mode
    resetCyclesTask = [this] { resetCycles(); };
    schedule(Task(&resetCyclesTask, 0x7FFFFFFF));
    gpu.scheduleInit();
    spu.scheduleInit();

    // Initialize the memory and CPUs
    memory.updateMap9(0x00000000, 0xFFFFFFFF);
    memory.updateMap7(0x00000000, 0xFFFFFFFF);
    interpreter[0].init();
    interpreter[1].init();

    if (!gbaPath.empty()) {
        // Load a GBA ROM
        if (!cartridgeGba.loadRom(gbaPath))
            throw ERROR_ROM;

        // Enable GBA mode right away if direct boot is enabled
        if (ndsPath.empty() && Settings::getDirectBoot()) {
            memory.write<uint16_t>(false, 0x4000304, 0x8003); // POWCNT1
            enterGbaMode();
        }
    }

    if (!ndsPath.empty()) {
        // Load an NDS ROM
        if (!cartridgeNds.loadRom(ndsPath))
            throw ERROR_ROM;

        // Prepare to boot the NDS ROM directly if direct boot is enabled
        if (Settings::getDirectBoot()) {
            // Set some registers as the BIOS/firmware would
            cp15.write(1, 0, 0, 0x0005707D); // CP15 Control
            cp15.write(9, 1, 0, 0x0300000A); // Data TCM base/size
            cp15.write(9, 1, 1, 0x00000020); // Instruction TCM size
            memory.write<uint8_t>(false, 0x4000247, 0x03); // WRAMCNT
            memory.write<uint8_t>(false, 0x4000300, 0x01); // POSTFLG (ARM9)
            memory.write<uint8_t>(true, 0x4000300, 0x01); // POSTFLG (ARM7)
            memory.write<uint16_t>(false, 0x4000304, 0x0001); // POWCNT1
            memory.write<uint16_t>(true, 0x4000504, 0x0200); // SOUNDBIAS

            // Set some memory values as the BIOS/firmware would
            memory.write<uint32_t>(false, 0x27FF800, 0x00001FC2); // Chip ID 1
            memory.write<uint32_t>(false, 0x27FF804, 0x00001FC2); // Chip ID 2
            memory.write<uint16_t>(false, 0x27FF850, 0x5835); // ARM7 BIOS CRC
            memory.write<uint16_t>(false, 0x27FF880, 0x0007); // Message from ARM9 to ARM7
            memory.write<uint16_t>(false, 0x27FF884, 0x0006); // ARM7 boot task
            memory.write<uint32_t>(false, 0x27FFC00, 0x00001FC2); // Copy of chip ID 1
            memory.write<uint32_t>(false, 0x27FFC04, 0x00001FC2); // Copy of chip ID 2
            memory.write<uint16_t>(false, 0x27FFC10, 0x5835); // Copy of ARM7 BIOS CRC
            memory.write<uint16_t>(false, 0x27FFC40, 0x0001); // Boot indicator

            cartridgeNds.directBoot();
            interpreter[0].directBoot();
            interpreter[1].directBoot();
            spi.directBoot();
        }
    }

    // Let the core run
    running.store(true);
}

void Core::resetCycles() {
    // Reset the global cycle count periodically to prevent overflow
    for (auto & task : tasks)
        task.cycles -= globalCycles;
    arm9Cycles -= std::min(globalCycles, arm9Cycles);
    arm7Cycles -= std::min(globalCycles, arm7Cycles);
    timers[0].resetCycles();
    timers[1].resetCycles();
    globalCycles -= globalCycles;
    schedule(Task(&resetCyclesTask, 0x7FFFFFFF));
}

void Core::runGbaFrame() {
    // Run a frame in GBA mode
    while (running.exchange(true)) {
        // Run the ARM7 until the next scheduled task
        if (arm7Cycles > globalCycles) globalCycles = arm7Cycles;
        while (interpreter[1].shouldRun() && tasks[0].cycles > arm7Cycles)
            arm7Cycles = (globalCycles += interpreter[1].runOpcode());

        // Jump to the next scheduled task
        globalCycles = tasks[0].cycles;

        // Run all tasks that are scheduled now
        while (tasks[0].cycles <= globalCycles) {
            (*tasks[0].task)();
            tasks.erase(tasks.begin());
        }
    }
}

void Core::runNdsFrame() {
    // Run a frame in NDS mode
    while (running.exchange(true)) {
        // Run the CPUs until the next scheduled task
        while (tasks[0].cycles > globalCycles) {
            // Run the ARM9
            if (interpreter[0].shouldRun() && globalCycles >= arm9Cycles)
                arm9Cycles = globalCycles + interpreter[0].runOpcode();

            // Run the ARM7 at half the speed of the ARM9
            if (interpreter[1].shouldRun() && globalCycles >= arm7Cycles)
                arm7Cycles = globalCycles + (interpreter[1].runOpcode() << 1);

            // Count cycles up to the next soonest event
            globalCycles = std::min<uint32_t>((interpreter[0].shouldRun() ? arm9Cycles : -1),
                                              (interpreter[1].shouldRun() ? arm7Cycles : -1));
        }

        // Jump to the next scheduled task
        globalCycles = tasks[0].cycles;

        // Run all tasks that are scheduled now
        while (tasks[0].cycles <= globalCycles) {
            (*tasks[0].task)();
            tasks.erase(tasks.begin());
        }
    }
}

void Core::schedule(Task task) {
    // Add a task to the scheduler, sorted by least to most cycles until execution
    task.cycles += globalCycles;
    auto it = std::upper_bound(tasks.cbegin(), tasks.cend(), task);
    tasks.insert(it, task);
}

void Core::enterGbaMode() {
    // Switch to GBA mode
    gbaMode = true;
    runFunc = &Core::runGbaFrame;
    running.store(false);

    // Reset the scheduler and schedule initial tasks for GBA mode
    tasks.clear();
    schedule(Task(&resetCyclesTask, 1));
    gpu.gbaScheduleInit();
    spu.gbaScheduleInit();

    // Reset the system for GBA mode
    memory.updateMap7(0x00000000, 0xFFFFFFFF);
    interpreter[1].init();
    interpreter[1].setBios(nullptr);
    rtc.reset();

    // Set VRAM blocks A and B to plain access mode
    // This is used by the GPU to access the VRAM borders
    memory.write<uint8_t>(false, 0x4000240, 0x80); // VRAMCNT_A
    memory.write<uint8_t>(false, 0x4000241, 0x80); // VRAMCNT_B
}

void Core::endFrame() {
    // Break execution at the end of a frame and count it
    running.store(false);
    fpsCount++;

    // Update the FPS and reset the counter every second
    std::chrono::duration<double> fpsTime = std::chrono::steady_clock::now() - lastFpsTime;
    if (fpsTime.count() >= 1.0f) {
        fps = fpsCount;
        fpsCount = 0;
        lastFpsTime = std::chrono::steady_clock::now();
    }

    // Schedule WiFi updates only when needed
    if (wifi.shouldSchedule())
        wifi.scheduleInit();
}

FORCE_INLINE int Interpreter::runOpcode() {
    // Push the next opcode through the pipeline
    uint32_t opcode = pipeline[0];
    pipeline[0] = pipeline[1];

    // Execute an instruction
    if (cpsr & BIT(5)) // THUMB mode
    {
        // Fill the pipeline, incrementing the program counter
        pipeline[1] = core->memory.read<uint16_t>(cpu, *registers[15] += 2);

        return (this->*thumbInstrs[(opcode >> 6) & 0x3FF])(opcode);
    } else // ARM mode
    {
        // Fill the pipeline, incrementing the program counter
        pipeline[1] = core->memory.read<uint32_t>(cpu, *registers[15] += 4);

        // Evaluate the current opcode's condition
        switch (condition[((opcode >> 24) & 0xF0) | (cpsr >> 28)]) {
            case 0:
                return 1;                      // False
            case 2:
                return handleReserved(opcode); // Reserved
            default:
                return (this->*armInstrs[((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0xF)])(opcode);
        }
    }
}
