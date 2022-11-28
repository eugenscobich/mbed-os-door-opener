#include "mbed.h"
#include "DoorController.h"
#include <cstdint>


void DoorController::handle() {
    handleStopSignals();
    handleCommand();
    handleCounter();
    handleCurrent();
}

void DoorController::stopAll() {
    pwmOut = 1.0; // 1.0 -> 0%, 
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 0;
    //stopReadCurrent();
}

void DoorController::currentReadTickerHandler() {
    readCurrent = true;
}

void DoorController::open() {
    currentDoorState = DOOR_STATE_OPEN;
}

void DoorController::close() {
    currentDoorState = DOOR_STATE_CLOSE;
}

void DoorController::stop() {
    currentDoorState = DOOR_STATE_STOP;
}

void DoorController::lock() {
    ThisThread::sleep_for(ACTION_DELAY_TIME);
    lockDelayed();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::lockDelayed), ACTION_DELAY_TIME);
}

void DoorController::lockDelayed() {
    motorActuatorRelayDigitalOut = 1;
    closeRelayDigitalOut = 1;
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME);
    startLockTimeoutHandler();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::startLockTimeoutHandler), CHANGE_RELAY_STATE_TIME);
}

void DoorController::startLockTimeoutHandler() {
    pwmOut = 0.0; // 100%
    currentDoorState = DOOR_STATE_LOCK;
    ThisThread::sleep_for(ACTUATOR_LOCK_UNLOCK_TIME);
    endLockTimeoutHandler();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::endLockTimeoutHandler), ACTUATOR_LOCK_UNLOCK_TIME);
}

void DoorController::endLockTimeoutHandler() {
    stopAll();
    currentDoorState = DOOR_STATE_LOCKED;
}

void DoorController::unlock() {
    ThisThread::sleep_for(ACTION_DELAY_TIME);
    unlockDelayed();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::unlockDelayed), ACTION_DELAY_TIME);
}

void DoorController::unlockDelayed() {
    motorActuatorRelayDigitalOut = 1;
    openRelayDigitalOut = 1;
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME);
    startUnlockTimeoutHandler();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::startUnlockTimeoutHandler), CHANGE_RELAY_STATE_TIME);
}

void DoorController::startUnlockTimeoutHandler() {
    pwmOut = 0.0; // 100%
    currentDoorState = DOOR_STATE_UNLOCK;
    ThisThread::sleep_for(ACTUATOR_LOCK_UNLOCK_TIME);
    endUnlockTimeoutHandler();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::endUnlockTimeoutHandler), ACTUATOR_LOCK_UNLOCK_TIME);
}

void DoorController::endUnlockTimeoutHandler() {
    stopAll();
    currentDoorState = DOOR_STATE_UNLOCKED;
}

uint16_t DoorController::getCount() {
    return count;
}

uint16_t DoorController::getCurrent() {
    uint16_t i = 0;
    for (uint8_t j = 0; j < 10; j++) {
        i = i + currents[j];
    }
    return i / 10;
}

uint8_t DoorController::getDoorState() {
    return currentDoorState;
}

void DoorController::counterIntrerruptHandler() {
    if(currentDoorState == DOOR_STATE_OPENING) {
        count++;
    } else if(currentDoorState == DOOR_STATE_CLOSING) {
        count--;
    }
}

void DoorController::openSignalIntrerruptHandler() {
    if (isDoorOpening() && isDoorOpened() && (currentDoorState == DOOR_STATE_OPEN || currentDoorState == DOOR_STATE_OPENING || currentDoorState == DOOR_STATE_STOPING)) {
        stopAll();
        stopReadCurrent();
        currentDoorState = DOOR_STATE_OPENED;
    }
    //stopAll();
    //timeout.detach();
    //currentDoorState = DOOR_STATE_OPENED;
}

void DoorController::closeSignalIntrerruptHandler() {
    if (isDoorClosing() && isDoorClosed() && (currentDoorState == DOOR_STATE_CLOSE || currentDoorState == DOOR_STATE_CLOSING || currentDoorState == DOOR_STATE_STOPING)) {
        stopAll();
        stopReadCurrent();
        currentDoorState = DOOR_STATE_CLOSED;
    }
    //stopAll();
    //timeout.detach();
    //currentDoorState = DOOR_STATE_CLOSED;
}


void DoorController::clearCurrent() {
    for (uint8_t i = 0; i < 10; i++) {
        currents[i] = 0;
    }
}

void DoorController::stopReadCurrent() {
    currentReadTicker.detach();
    clearCurrent();
}

void DoorController::startReadCurrent() {
    currentReadTicker.attach(mbed::callback(this, &DoorController::currentReadTickerHandler), 100ms);
}

void DoorController::handleCurrent() {
    if (readCurrent) {
        currents[currentsArrayPointer++] = currentSensorAnalogIn.read_u16() / 20;
        if (currentsArrayPointer == 10) {
            currentsArrayPointer = 0;
        }
        readCurrent = false;
    }

    if (isDoorOpening() || isDoorClosing()) {
        if (getCurrent() > currentTrashhold) {
            //stopAll();
            //stopReadCurrent();
            currentDoorState = DOOR_STATE_ALARM;
        }
    }
}

void DoorController::handleCommand() {
    // Handle comands
    if (previousDoorState != currentDoorState && currentDoorState != DOOR_STATE_UNKNOWN) {
        previousDoorState = currentDoorState;
        if (currentDoorState == DOOR_STATE_OPEN) {            
            stopAll();
            openAnotherDoorCallbackCalled = false;
            closeAnotherDoorCallbackCalled = false;
            openedCallbackCalled = false;
            closedCallbackCalled = false;
            stopedCallbackCalled = false;
            alarmCallbackCalled = true;
            armed = false;
            if (!isDoorOpened()) {
                ThisThread::sleep_for(ACTION_DELAY_TIME);
                startOpening();
                //timeout.detach();
                //timeout.attach(mbed::callback(this, &DoorController::startOpening), ACTION_DELAY_TIME);
            } else {
                currentDoorState = DOOR_STATE_OPENED;
            }
        } else if (currentDoorState == DOOR_STATE_CLOSE) {
            stopAll();
            openAnotherDoorCallbackCalled = false;
            closeAnotherDoorCallbackCalled = false;
            openedCallbackCalled = false;
            closedCallbackCalled = false;
            stopedCallbackCalled = false;
            alarmCallbackCalled = true;
            armed = false;
            if (!isDoorClosed()) {
                ThisThread::sleep_for(ACTION_DELAY_TIME);
                startClosing();
                //timeout.detach();
                //timeout.attach(mbed::callback(this, &DoorController::startClosing), ACTION_DELAY_TIME);
            } else {
                currentDoorState = DOOR_STATE_CLOSED;
            }
        } else if (currentDoorState == DOOR_STATE_STOP) {
            if (!isDoorOpened() && !isDoorClosed()) {
                startStoping();
            } else {
                currentDoorState = DOOR_STATE_STOPED;
            }
        } else if (currentDoorState == DOOR_STATE_STOPING) {
            if (!isDoorOpened() && !isDoorClosed()) {
                startStoping(false);
            }
        } else if (currentDoorState == DOOR_STATE_OPENED) {
            if (!openedCallbackCalled) {
                if (openedCallback) {
                    openedCallback.call();
                }
                openedCallbackCalled = true;
            }   
        } else if (currentDoorState == DOOR_STATE_CLOSED) {
            if (!closedCallbackCalled) {
                if (closedCallback) {
                    closedCallback.call();
                }
                closedCallbackCalled = true;
            } 
        } else if (currentDoorState == DOOR_STATE_STOPED) {
            if (!stopedCallbackCalled) {
                if (stopedCallback) {
                    stopedCallback.call();
                }
                stopedCallbackCalled = true;
            }
        } else if (currentDoorState == DOOR_STATE_LOCKED) {
            if (lockAnotherDoorCallback) {
                lockAnotherDoorCallback.call();
                alarmCallbackCalled = false;
                armed = true;
            }
        } else if (currentDoorState == DOOR_STATE_UNLOCKED) {
            if (unlockAnotherDoorCallback) {
                unlockAnotherDoorCallback.call();
            }
        } else if (currentDoorState == DOOR_STATE_ALARM) {
            if (!alarmCallbackCalled) {
                if (alarmCallback) {
                    alarmCallback.call();
                }
            }
        }
    }
}

void DoorController::handleCounter() {
    if (currentDoorState == DOOR_STATE_OPENING && count >= COUNTER_DELTA || currentDoorState == DOOR_STATE_OPENED) {
        if (!openAnotherDoorCallbackCalled) {
            if (openAnotherDoorCallback) {
                openAnotherDoorCallback.call();
            }
            openAnotherDoorCallbackCalled = true;
        }   
    }

    if (currentDoorState == DOOR_STATE_CLOSING && count <= maxCount - COUNTER_DELTA  || currentDoorState == DOOR_STATE_CLOSED) {
        if (!closeAnotherDoorCallbackCalled) {
            if (closeAnotherDoorCallback) {
                closeAnotherDoorCallback.call();
            }
            closeAnotherDoorCallbackCalled = true;
        }
    }

    if (currentDoorState == DOOR_STATE_OPENING && count >= maxCount - COUNTER_DELTA) {
        currentDoorState = DOOR_STATE_STOPING;
    }

    if (currentDoorState == DOOR_STATE_CLOSING && count <= COUNTER_DELTA) {
        currentDoorState = DOOR_STATE_STOPING;
    }
}

void DoorController::speedUpTickerHandler() {
    if ((isDoorOpening() || isDoorClosing()) && pwmOut > 0.0) {
        pwmOut = pwmOut - 0.01;
    } else {
        speedTicker.detach();
        stopReadCurrent();
        startReadCurrent();
        if (isDoorOpening()) {
            currentDoorState = DOOR_STATE_OPENING;
        }
        if (isDoorClosing()) {
            currentDoorState = DOOR_STATE_CLOSING;
        }
    }
}

void DoorController::speedUpTimeoutHandler() {
    if (isDoorOpening() || isDoorClosing()) {
        pwmOut = 0.3; // 70%
        speedTicker.attach(mbed::callback(this, &DoorController::speedUpTickerHandler), PWM_CHANGE_SPEED_TIME);
    }
}

void DoorController::startOpening() {
    if (isDoorClosed()) {
        count = 0;
    }
    pwmOut = 1.0; // 1.0 -> 0%
    closeRelayDigitalOut = 0;
    openRelayDigitalOut = 1;
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME);
    speedUpTimeoutHandler();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::speedUpTimeoutHandler), CHANGE_RELAY_STATE_TIME); // Wait relay to change the state
    printf("Start Opening\n");
}

void DoorController::startClosing() {
    if (isDoorOpened()) {
        count = maxCount;
    }
    pwmOut = 1.0; // 1.0 -> 0%
    closeRelayDigitalOut = 1;
    openRelayDigitalOut = 0;
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME);
    speedUpTimeoutHandler();
    //timeout.detach();
    //timeout.attach(mbed::callback(this, &DoorController::speedUpTimeoutHandler), CHANGE_RELAY_STATE_TIME); // Wait relay to change the state
    printf("Start Closing\n");
}

void DoorController::startStoping(bool fullStop) {
    stopedCallbackCalled = false;
    fullStopFlag = fullStop;
    //timeout.detach();
    //stopReadCurrent();
    speedTicker.attach(mbed::callback(this, &DoorController::speedDownTickerHandler), PWM_CHANGE_SPEED_TIME);
    printf("Start Stoping\n");
}

void DoorController::speedDownTickerHandler() {
    if ((isDoorOpening() || isDoorClosing()) && pwmOut < 0.3) {
        pwmOut = pwmOut + 0.01;
    } else {
        if (fullStopFlag == true) {
            stopAll();
            stopReadCurrent();
            currentDoorState = DOOR_STATE_STOPED;
        }
        speedTicker.detach();
    }
}

void DoorController::handleStopSignals() {
    if (isDoorOpening() && isDoorOpened() && (currentDoorState == DOOR_STATE_OPEN || currentDoorState == DOOR_STATE_OPENING || currentDoorState == DOOR_STATE_STOPING)) {
        stopAll();
        stopReadCurrent();
        currentDoorState = DOOR_STATE_OPENED;
    }

    if (isDoorClosing() && isDoorClosed() && (currentDoorState == DOOR_STATE_CLOSE || currentDoorState == DOOR_STATE_CLOSING || currentDoorState == DOOR_STATE_STOPING)) {
        stopAll();
        stopReadCurrent();
        currentDoorState = DOOR_STATE_CLOSED;
    }

    if (armed && !isDoorClosed()) {
        currentDoorState = DOOR_STATE_ALARM;
    }

}

bool DoorController::isDoorOpened() {
    return !openSignalDebounceIn.read();
}

bool DoorController::isDoorClosed() {
    return !closeSignalDebounceIn.read();
}

bool DoorController::isDoorOpening() {
    return openRelayDigitalOut.read();
}

bool DoorController::isDoorClosing() {
    return closeRelayDigitalOut.read();
}

void DoorController::printDoorState(char* buff)
{
    if (currentDoorState == DOOR_STATE_UNKNOWN) {
        strncpy(buff, "Unknown", 8);
    } else if (currentDoorState == DOOR_STATE_OPEN) {
        strncpy(buff, "Open", 8);
    } else if (currentDoorState == DOOR_STATE_CLOSE) {
        strncpy(buff, "Close", 8);
    } else if (currentDoorState == DOOR_STATE_OPENED) {
        strncpy(buff, "Opened", 8);
    } else if (currentDoorState == DOOR_STATE_CLOSED) {
        strncpy(buff, "Closed", 8);
    } else if (currentDoorState == DOOR_STATE_OPENING) {
        strncpy(buff, "Opening", 8);
    } else if (currentDoorState == DOOR_STATE_CLOSING) {
        strncpy(buff, "Closing", 8);
    } else if (currentDoorState == DOOR_STATE_LOCK) {
        strncpy(buff, "Lock", 8);
    } else if (currentDoorState == DOOR_STATE_UNLOCK) {
        strncpy(buff, "Unlock", 8);
    } else if (currentDoorState == DOOR_STATE_LOCKED) {
        strncpy(buff, "Locked", 8);
    } else if (currentDoorState == DOOR_STATE_UNLOCKED) {
        strncpy(buff, "Unlocked", 8);
    }else if (currentDoorState == DOOR_STATE_STOP) {
        strncpy(buff, "Stop", 8);
    } else if (currentDoorState == DOOR_STATE_STOPED) {
        strncpy(buff, "Stoped", 8);
    } else if (currentDoorState == DOOR_STATE_STOPING) {
        strncpy(buff, "Stoping", 8);
    } else if (currentDoorState == DOOR_STATE_ALARM) {
        strncpy(buff, "Alarm", 8);
    }
}

void DoorController::setOpenedCallback(Callback<void()> callback) {
    openedCallback = callback;
}

void DoorController::setClosedCallback(Callback<void()> callback) {
    closedCallback = callback;
}

void DoorController::setStopedCallback(Callback<void()> callback) {
    stopedCallback = callback;
}

void DoorController::setAlarmCallback(Callback<void()> callback) {
    alarmCallback = callback;
}

void DoorController::setOpenAnotherDoorCallback(Callback<void()> callback) {
    openAnotherDoorCallback = callback;
}

void DoorController::setCloseAnotherDoorCallback(Callback<void()> callback) {
    closeAnotherDoorCallback = callback;
}

void DoorController::setLockAnotherDoorCallback(Callback<void()> callback) {
    lockAnotherDoorCallback = callback;
}

void DoorController::setUnlockAnotherDoorCallback(Callback<void()> callback) {
    unlockAnotherDoorCallback = callback;
}