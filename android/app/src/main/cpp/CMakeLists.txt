cmake_minimum_required(VERSION 3.25.1)

project(emuDeeS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -flto")

add_library(emuDeeS SHARED
        interface.cpp
        nds_icon.cpp
        screen_layout.cpp
        bios.cpp
        cartridge.cpp
        core.cpp
        cp15.cpp
        div_sqrt.cpp
        dldi.cpp
        dma.cpp
        gpu.cpp
        gpu_2d.cpp
        gpu_3d.cpp
        gpu_3d_renderer.cpp
        input.cpp
        interpreter.cpp
        interpreter_alu.cpp
        interpreter_branch.cpp
        interpreter_transfer.cpp
        ipc.cpp
        memory.cpp
        rtc.cpp
        settings.cpp
        spi.cpp
        spu.cpp
        timers.cpp
        wifi.cpp)

target_link_libraries(emuDeeS jnigraphics OpenSLES)
