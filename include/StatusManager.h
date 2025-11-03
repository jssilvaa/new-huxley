// StatusManager.h
#pragma once

#include <pthread.h>

// Controls RGB LED status indicators on the embedded platform.
class StatusManager {
public:
    enum class State { Booting, Operational, Processing, Error };
    enum class LedColor { Off, Green, Yellow, Red };

    StatusManager();
    ~StatusManager();

    void setState(State state);
    void setLedColor(LedColor color);

private:
    void applyStateLocked(State state);
    void driveHardware(LedColor color);

    pthread_mutex_t ledMutex;
    State currentState;
    LedColor currentColor;
};
