#include "mbed.h"
#include "DoorController.h"
#include <cstdint>


void DoorController::doorThreadHandler()
{
    while(1) {

        // Doublecheck sinals
        if (currentDoorState != DOOR_STATE_LOCK && 
            currentDoorState != DOOR_STATE_UNLOCK && 
            currentDoorState != DOOR_STATE_LOCKED && 
            currentDoorState != DOOR_STATE_UNLOCKED && 
            openRelayDigitalOut == 1 &&
            openSignalDebounceIn.read() == 0)
        {
            stopAll();
        }

        if (currentDoorState != DOOR_STATE_LOCK && 
            currentDoorState != DOOR_STATE_UNLOCK && 
            currentDoorState != DOOR_STATE_LOCKED && 
            currentDoorState != DOOR_STATE_UNLOCKED && 
            closeRelayDigitalOut == 1 &&
            closeSignalDebounceIn.read() == 0)
        {
            stopAll();
        }

        
        // Check Alarm sinals
        if (currentDoorState == DOOR_STATE_LOCKED &&
            closeSignalDebounceIn.read() == 1) {
            if (alarmCallback && alarmCallbackCalled == false) {
                alarmCallback.call();
                alarmCallbackCalled = true;
            }
        }

        if (currentDoorState != DOOR_STATE_LOCK && 
            currentDoorState != DOOR_STATE_UNLOCK && 
            currentDoorState != DOOR_STATE_LOCKED && 
            currentDoorState != DOOR_STATE_UNLOCKED && 
            closeRelayDigitalOut == 1 &&
            closeSignalDebounceIn.read() == 0)
        {
            stopAll();
        }

        // Handle comands
        if (previousDoorState != currentDoorState && currentDoorState != DOOR_STATE_UNKNOWN) {
            previousDoorState = currentDoorState;
            if (currentDoorState == DOOR_STATE_OPEN) {
                openAnotherDoorCallbackCalled = false;
                closeAnotherDoorCallbackCalled = false;
                openedCallbackCalled = false;
                closedCallbackCalled = false;
                stopedCallbackCalled = false;
                if (openSignalDebounceIn.read() == 0) {
                    currentDoorState = DOOR_STATE_OPENED;
                } else {
                    pwmOut = 1.0; // 1.0 -> 0%
                    closeRelayDigitalOut = 0;
                    openRelayDigitalOut = 1;
                    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME); // Wait relay to change the state
                    speedUp(70);
                    if (currentDoorState == DOOR_STATE_OPEN) {
                        currentDoorState = DOOR_STATE_OPENING;
                    }
                }
            } else if (currentDoorState == DOOR_STATE_CLOSE) {
                openAnotherDoorCallbackCalled = false;
                closeAnotherDoorCallbackCalled = false;
                openedCallbackCalled = false;
                closedCallbackCalled = false;
                stopedCallbackCalled = false;
                if (closeSignalDebounceIn.read() == 0) {
                    currentDoorState = DOOR_STATE_CLOSED;
                } else {
                    pwmOut = 1.0; // 1.0 -> 0%
                    openRelayDigitalOut = 0;
                    closeRelayDigitalOut = 1;
                    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME); // Wait relay to change the state
                    speedUp(70);
                    if (currentDoorState == DOOR_STATE_CLOSE) {
                        currentDoorState = DOOR_STATE_CLOSING;    
                    }
                }
            } else if (currentDoorState == DOOR_STATE_STOP) {
                if (openRelayDigitalOut == 1 || closeRelayDigitalOut == 1) {
                    speedDown(50);
                    pwmOut = 1.0; // 0%
                    openRelayDigitalOut = 0;
                    closeRelayDigitalOut = 0;
                }
                if (currentDoorState == DOOR_STATE_STOP) {
                    currentDoorState = DOOR_STATE_STOPED;     
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
            } else if (currentDoorState == DOOR_STATE_STOPING) {
                speedDown(70);
            }
        }
        

        if (currentDoorState == DOOR_STATE_OPENING && count >= 10 || currentDoorState == DOOR_STATE_OPENED) {
            if (!openAnotherDoorCallbackCalled) {
                if (openAnotherDoorCallback) {
                    openAnotherDoorCallback.call();
                }
                openAnotherDoorCallbackCalled = true;
            }   
        }

        if (currentDoorState == DOOR_STATE_CLOSING && count <= maxCount - 10  || currentDoorState == DOOR_STATE_CLOSED) {
            if (!closeAnotherDoorCallbackCalled) {
                if (closeAnotherDoorCallback) {
                    closeAnotherDoorCallback.call();
                }
                closeAnotherDoorCallbackCalled = true;
            }
        }

        if (currentDoorState == DOOR_STATE_OPENING && count >= maxCount - 10) {
            currentDoorState = DOOR_STATE_STOPING;
        }

        if (currentDoorState == DOOR_STATE_CLOSING && count <= 10) {
            currentDoorState = DOOR_STATE_STOPING;
        }


        if (currentDoorState == DOOR_STATE_OPEN || currentDoorState == DOOR_STATE_OPENING || currentDoorState == DOOR_STATE_CLOSE || currentDoorState == DOOR_STATE_CLOSING) {
            /*
            if (getCurrent() > 300) {
                openRelayDigitalOut = 0;
                closeRelayDigitalOut = 0;
                currentDoorState = DOOR_STATE_STOP;
            }
            */
        }

        if (readCurrent) {
            currents[currentsArrayPointer++] = currentSensorAnalogIn.read_u16() / 20;
            if (currentsArrayPointer == 10) {
                currentsArrayPointer = 0;
            }
            
            readCurrent = false;
        }
        ThisThread::sleep_for(10ms);
    }
}

void DoorController::stopAll() {
    pwmOut = 1.0; // 1.0 -> 0%, 
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 0;
}

void DoorController::speedUp(int procentageFrom) {
    // 0.0 -> 100%, 
    // 1.0 -> 0%, 
    // 0.8 -> 20%
    // 0.5 -> 50%
    for (unsigned short i = procentageFrom; i <= 100; i++) {
        if (closeRelayDigitalOut == 1 || openRelayDigitalOut == 1) {
            pwmOut = 1.0 - i / 100.0;
            ThisThread::sleep_for(PWM_CHANGE_SPEED_TIME);
        } else {
            break;
        }
    }
}

void DoorController::speedDown(int procentageTo) {
    // 0.0 -> 100%, 
    // 1.0 -> 0%, 
    // 0.8 -> 20%
    // 0.5 -> 50%
    float currentPwmValue = pwmOut.read();
    for (unsigned short i = 100 - currentPwmValue * 100; i >= procentageTo; i--) {
        if (closeRelayDigitalOut == 1 || openRelayDigitalOut == 1) {
            pwmOut = 1.0 - i / 100.0;
            ThisThread::sleep_for(PWM_CHANGE_SPEED_TIME);
        } else {
            break;
        }
    }
}

void DoorController::currentReadTickerHandler() {
    readCurrent = true;
}

void DoorController::setOpenAnotherDoorCallback(Callback<void()> callback) {
    openAnotherDoorCallback = callback;
}


void DoorController::setCloseAnotherDoorCallback(Callback<void()> callback) {
    closeAnotherDoorCallback = callback;
}


void DoorController::open() {
    if (closeSignalDebounceIn == 1) {
        count = 0;
    }
    currentDoorState = DOOR_STATE_OPEN;
}


void DoorController::close() {
    if (openSignalDebounceIn == 1) {
        count = maxCount;
    }
    currentDoorState = DOOR_STATE_CLOSE;
}

void DoorController::stop()
{
    currentDoorState = DOOR_STATE_STOP;
}


void DoorController::lockTimeoutHandler()
{
    stopAll();
    currentDoorState = DOOR_STATE_LOCKED;
    alarmCallbackCalled = false;
}

void DoorController::unlockTimeoutHandler()
{
    stopAll();
    currentDoorState = DOOR_STATE_UNLOCKED;
}

void DoorController::lock()
{
    stopAll();
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME);
    motorActuatorRelayDigitalOut = 1;
    closeRelayDigitalOut = 1;
    motorActuatorRelayDigitalOut = 1;
    motorActuatorRelayDigitalOut = 1;
    motorActuatorRelayDigitalOut = 1;
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME); // Wait relay to change the state
    pwmOut = 0.0; // 100%
    currentDoorState = DOOR_STATE_LOCK;
    timeout.attach(mbed::callback(this, &DoorController::lockTimeoutHandler), ACTUATOR_LOCK_UNLOCK_TIME);
}

void DoorController::unlock()
{
    stopAll();
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME);
    motorActuatorRelayDigitalOut = 1;
    openRelayDigitalOut = 1;
    ThisThread::sleep_for(CHANGE_RELAY_STATE_TIME); // Wait relay to change the state
    pwmOut = 0.0; // 100%
    currentDoorState = DOOR_STATE_UNLOCK;
    timeout.attach(mbed::callback(this, &DoorController::unlockTimeoutHandler), ACTUATOR_LOCK_UNLOCK_TIME);
}

uint16_t DoorController::getCount() {
    return count;
}

uint16_t DoorController::getCurrent() {
    uint32_t i = 0;
    for (uint8_t j = 0; j < 10; j++) {
        i = i + currents[j];
    }
    return i / 10;
}

uint8_t DoorController::getDoorState()
{
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
    stopAll();
    timeout.detach();
    currentDoorState = DOOR_STATE_OPENED;
}

void DoorController::closeSignalIntrerruptHandler() {
    stopAll();
    timeout.detach();
    currentDoorState = DOOR_STATE_CLOSED;
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