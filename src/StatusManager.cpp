#include "StatusManager.h"

#include <iostream>

namespace {
StatusManager::LedColor colorForState(StatusManager::State state)
{
    switch (state) {
    case StatusManager::State::Booting: return StatusManager::LedColor::Yellow;
    case StatusManager::State::Operational: return StatusManager::LedColor::Green;
    case StatusManager::State::Processing: return StatusManager::LedColor::Yellow;
    case StatusManager::State::Error: return StatusManager::LedColor::Red;
    }
    return StatusManager::LedColor::Off;
}

const char* colorName(StatusManager::LedColor color)
{
    switch (color) {
    case StatusManager::LedColor::Off: return "off";
    case StatusManager::LedColor::Green: return "green";
    case StatusManager::LedColor::Yellow: return "yellow";
    case StatusManager::LedColor::Red: return "red";
    }
    return "unknown";
}
} // namespace

StatusManager::StatusManager()
    : currentState(State::Booting)
    , currentColor(LedColor::Off)
{
    pthread_mutex_init(&ledMutex, nullptr);
}

StatusManager::~StatusManager()
{
    pthread_mutex_destroy(&ledMutex);
}

void StatusManager::setState(State state)
{
    pthread_mutex_lock(&ledMutex);
    currentState = state;
    applyStateLocked(state);
    pthread_mutex_unlock(&ledMutex);
}

void StatusManager::setLedColor(LedColor color)
{
    pthread_mutex_lock(&ledMutex);
    currentColor = color;
    driveHardware(color);
    pthread_mutex_unlock(&ledMutex);
}

void StatusManager::applyStateLocked(State state)
{
    const auto nextColor = colorForState(state);
    if (nextColor != currentColor) {
        currentColor = nextColor;
        driveHardware(currentColor);
    }
}

void StatusManager::driveHardware(LedColor color)
{
    // Stub implementation – replace with GPIO writes on target platform.
    std::cout << "[LED] set color: " << colorName(color) << std::endl;
}
