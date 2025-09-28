// StatusManager.h
#pragma once
#include <string>

class StatusManager {
public:
    enum class State { BOOTING, OPERATIONAL, PROCESSING, ERROR };

    StatusManager();
    void setState(State s);

private:
    void updateLed(State s);
};
