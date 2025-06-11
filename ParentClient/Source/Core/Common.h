#pragma once

#include <thread>
#include <vector>
#include <mutex>

// Current
extern std::mutex g_framePixelsMutex;
extern std::vector<uint8_t> g_framePixelsData;

extern std::mutex g_frameSizeMutex;
extern uint32_t g_frameHeight;
extern uint32_t g_frameWidth;

extern std::mutex g_frameQualityMutex;
extern uint32_t g_frameQuality;

// Temp
extern uint32_t g_newFrameQuality;
extern uint32_t g_currentFrameWidth;
extern uint32_t g_currentFrameHeight;