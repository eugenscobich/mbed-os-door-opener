#include "mbed.h"
#include "DoorController.h"
#include <cstdint>


void DoorController::doorThreadHandler()
{
    while(1) {

        if (previousDoorState != currentDoorState && currentDoorState != DOOR_STATE_UNKNOWN) {
            previousDoorState = currentDoorState;
            if (currentDoorState == DOOR_STATE_OPEN) {
                openAnotherDoorCallbackCalled = false;
                closeAnotherDoorCallbackCalled = false;
                if (openSignalDebounceIn.read() == 0) {
                    currentDoorState = DOOR_STATE_OPENED;
                } else {
                    pwmOut.write(0.2);
                    closeRelayDigitalOut = 0;
                    openRelayDigitalOut = 1;
                    for (unsigned short i = 20; i <= 100; i++) {
                        pwmOut.write(i / 100);
                        ThisThread::sleep_for(10ms);
                    }
                    currentDoorState = DOOR_STATE_OPENING;
                }
            } else if (currentDoorState == DOOR_STATE_CLOSE) {
                openAnotherDoorCallbackCalled = false;
                closeAnotherDoorCallbackCalled = false;
                if (closeSignalDebounceIn.read() == 0) {
                    currentDoorState = DOOR_STATE_CLOSED;
                } else {
                    pwmOut.write(0.2);
                    openRelayDigitalOut = 0;
                    closeRelayDigitalOut = 1;
                    for (unsigned short i = 20; i <= 100; i++) {
                        pwmOut.write(i / 100);
                        ThisThread::sleep_for(10ms);
                    }
                    currentDoorState = DOOR_STATE_CLOSING;    
                }            
            } else if (currentDoorState == DOOR_STATE_STOP) {
                if (openRelayDigitalOut == 1 || closeRelayDigitalOut == 1) {
                    if (pwmOut.read() == 1.0) {
                         for (unsigned short i = 100; i > 20; i--) {
                            pwmOut.write(i / 100);
                            ThisThread::sleep_for(10ms);
                        }
                    }
                    openRelayDigitalOut = 0;
                    closeRelayDigitalOut = 0;
                }
                currentDoorState = DOOR_STATE_STOPED;                
            } else if (currentDoorState == DOOR_STATE_OPENED) {
                openedCallback.call();
            } else if (currentDoorState == DOOR_STATE_CLOSED) {
                closedCallback.call();
            } else if (currentDoorState == DOOR_STATE_STOPED) {
                stopedCallback.call();
            } else if (currentDoorState == DOOR_STATE_STOPING) {
                for (unsigned short i = 100; i > 50; i--) {
                    pwmOut.write(i / 100);
                    ThisThread::sleep_for(10ms);
                }
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

/*
        if (currentDoorState == DOOR_STATE_OPEN || currentDoorState == DOOR_STATE_OPENING || currentDoorState == DOOR_STATE_CLOSE || currentDoorState == DOOR_STATE_CLOSING) {
            if (getCurrent() > 300) {
                openRelayDigitalOut = 0;
                closeRelayDigitalOut = 0;
                currentDoorState = DOOR_STATE_STOP;
            }
        }

        if (readCurrent) {
            
            currents[currentsArrayPointer++] = currentSensorAnalogIn.read_u16() / 20;
            if (currentsArrayPointer == 10) {
                currentsArrayPointer = 0;
            }
            
            readCurrent = false;
        }
        */
    }
}
/*
void DoorController::currentReadTickerHandler() {
    readCurrent = true;
}
*/
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
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 0;
    currentDoorState = DOOR_STATE_LOCKED;
}

void DoorController::unlockTimeoutHandler()
{
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 0;
    currentDoorState = DOOR_STATE_UNLOCKED;
}

void DoorController::lock()
{
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 1;
    closeRelayDigitalOut = 1;
    currentDoorState = DOOR_STATE_LOCK;
    timeout.attach(mbed::callback(this, &DoorController::lockTimeoutHandler), 3000ms);
}

void DoorController::unlock()
{
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 1;
    openRelayDigitalOut = 1;
    currentDoorState = DOOR_STATE_UNLOCK;
    timeout.attach(mbed::callback(this, &DoorController::unlockTimeoutHandler), 3000ms);
}

uint16_t DoorController::getCount()
{
    return count;
}

uint16_t DoorController::getCurrent() {
    return currentSensorAnalogIn.read_u16() / 20; 
    /*
    uint32_t i = 0;
    for (uint8_t j = 0; j < 10; j++) {
        i = i + currents[j];
    }
    return i / 10; */
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
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 0;
    currentDoorState = DOOR_STATE_OPENED;
}

void DoorController::closeSignalIntrerruptHandler() {
    openRelayDigitalOut = 0;
    closeRelayDigitalOut = 0;
    motorActuatorRelayDigitalOut = 0;
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