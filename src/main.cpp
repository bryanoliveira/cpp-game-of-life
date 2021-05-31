/**
 * Author: Bryan Lincoln
 * Email: bryanufg@gmail.com
 *
 * Using ISO C++ 17 (C++ 11 may be compatible)
 *
 * Conventions (a variation of STL/Boost Style Guides):
 *  - use spaces instead of tabs
 *  - indent with 4 spaces
 *  - variables are camelCased
 *    - params are prefixed with p (e.g. pFillProb)
 *    - member variables are prefixed with m (e.g. mFillProb)
 *    - globals are prefixed with g (e.g. gDisplay)
 *       - the 'config' namespace doesn't follow this as the 'config::' prefix
 *         is always made explicit
 *  - methods are snake_cased
 *  - CUDA kernels are prefixed with k (e.g. k_compute_grid())
 *  - Macros are UPPER_CASED (e.g. CUDA_ASSERT())
 */
#include <chrono>
#include <iostream>
#include <thread>
#include <signal.h>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include "automata_interface.hpp"
#include "automata_base_cpu.hpp"
#include "automata_base_gpu.cuh"
#include "config.hpp"
#include "display.hpp"
#include "pattern.hpp"
#include "stats.hpp"

Display *gDisplay;
AutomataInterface *gAutomata;

bool gLooping = true;
unsigned long gLastIterationCount = 0;
unsigned long gIterationsPerSecond = 0;
unsigned long gNsBetweenSeconds = 0;
std::chrono::steady_clock::time_point gLastPrintClock =
    std::chrono::steady_clock::now();
std::ostringstream gLiveLogBuffer;

void loop();
bool should_log();
void live_log();
void sigint_handler(int s);

int main(int argc, char **argv) {
    spdlog::cfg::load_env_levels();

    const unsigned long randSeed = time(nullptr);
    struct sigaction sigIntHandler;

    // configure interrupt signal handler
    sigIntHandler.sa_handler = sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    // load command line arguments
    config::load_cmd(argc, argv);
    controls::paused = config::startPaused;

    // configure display
    if (config::render)
        gDisplay = new Display(&argc, argv, loop, config::cpuOnly);

    // configure automata object
    if (config::cpuOnly)
        // the CPU implementation uses the buffer update function provided by
        // the display class and we configure it here to reduce complexity by
        // maintaining the AutomataInterface predictable
        gAutomata = static_cast<AutomataInterface *>(
            new cpu::AutomataBase(randSeed, &gLiveLogBuffer, []() {
                gDisplay->update_grid_buffers_cpu();
            }));
    else if (config::render)
        // the GPU implementation updates the VBO using the CUDA<>GL interop
        gAutomata = static_cast<AutomataInterface *>(new gpu::AutomataBase(
            randSeed, &gLiveLogBuffer, &(gDisplay->grid_vbo())));
    else
        gAutomata = static_cast<AutomataInterface *>(
            new gpu::AutomataBase(randSeed, &gLiveLogBuffer));

    if (config::patternFileName != "random")
        load_pattern(config::patternFileName);

    if (config::render)
        gDisplay->start();
    else {
        while (gLooping)
            loop();
    }

    if (config::benchmarkMode) {
        stats::print_timings();
    } else {
        std::cout << std::endl;
        spdlog::info("Exiting after {} iterations.", stats::iterations);
    }

    // clean up
    delete gAutomata;

    if (config::render)
        delete gDisplay;

    return 0;
}

void loop() {
    // limit framerate
    if (config::renderDelayMs > 0)
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config::renderDelayMs));

    // loop timer
    const std::chrono::steady_clock::time_point timeStart =
        std::chrono::steady_clock::now();

    // prepare logging
    const bool logEnabled = !config::benchmarkMode && should_log();
    if (logEnabled)
        // carriage return
        gLiveLogBuffer << "\r\e[KIt: " << stats::iterations;

    // update buffers & render
    if (config::render) {
        // update display buffers
        gAutomata->update_grid_buffers();

        // display current grid
        gDisplay->draw(logEnabled, gIterationsPerSecond);
    }

    // compute next grid
    if (!controls::paused || controls::singleStep) {
        gAutomata->compute_grid(logEnabled); // count alive cells if will log
        stats::iterations++;
    } else if (!config::benchmarkMode) {
        std::cout << "\r\e[KPaused. Press space to resume." << std::flush;
    }
    controls::singleStep = false;

    // calculate loop time and iterations per second
    gNsBetweenSeconds += std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() - timeStart)
                             .count();
    if (logEnabled)
        live_log();

    // check if number of iterations reached max
    if (!gLooping || (config::maxIterations > 0 &&
                      stats::iterations >= config::maxIterations)) {
        if (config::render)
            gDisplay->stop();
        else
            gLooping = false;
    }
}

bool should_log() {
    // calculate loop time and iterations per second
    gIterationsPerSecond = stats::iterations - gLastIterationCount;
    // return if it's not time to update the log
    if (gIterationsPerSecond <= 0 ||
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - gLastPrintClock)
                .count() < 1)
        return false;
    return true;
}

void live_log() {
    // add main loop info to the buffer
    gLiveLogBuffer << " | It/s: " << gIterationsPerSecond
                   << " | Main Loop: "
                   // average time per iteration
                   << gNsBetweenSeconds / gIterationsPerSecond << " ns";
    // print the buffer
    std::cout << gLiveLogBuffer.str() << std::flush;
    // reset the buffer
    gLiveLogBuffer.str("");
    gLiveLogBuffer.clear();
    // update global counters
    gNsBetweenSeconds = 0;
    gLastIterationCount = stats::iterations;
    gLastPrintClock = std::chrono::steady_clock::now();
}

void sigint_handler(int s) {
    gLooping = false;
    std::cout << std::endl;
}