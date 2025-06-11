#include "Core/Common.h"

// Current
std::mutex g_framePixelsMutex;
std::vector<uint8_t> g_framePixelsData;

std::mutex g_frameSizeMutex;
uint32_t g_frameHeight = 0;
uint32_t g_frameWidth = 0;

std::mutex g_frameQualityMutex;
uint32_t g_frameQuality = 50;

// Temp
uint32_t g_newFrameQuality = 50;
uint32_t g_currentFrameWidth = 0;
uint32_t g_currentFrameHeight = 0;