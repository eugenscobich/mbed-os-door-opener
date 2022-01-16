#include "SignalLampController.h"
#include "mbed.h"

void SignalLampController::signalLampThreadHandler() {
    while(1) {
        if (isSignalLampOn) {
            signalLampRelayDigitalOut = !signalLampRelayDigitalOut;
            buzzerRelayDigitalOut = !buzzerRelayDigitalOut;
        }
        ThisThread::sleep_for(signalLampInterval);
    }
}

void SignalLampController::start() {
    signalLampRelayDigitalOut = 1;
    buzzerRelayDigitalOut = 1;
    isSignalLampOn = true;
}

void SignalLampController::stop() {
    signalLampRelayDigitalOut = 0;
    buzzerRelayDigitalOut = 0;
    isSignalLampOn = false;
}

void SignalLampController::alarm() {
    signalLampRelayDigitalOut = 1;
    buzzerRelayDigitalOut = 1;
}