#include "SignalLampController.h"
#include "mbed.h"

void SignalLampController::signalLampTickerHandler() {
    signalLampRelayDigitalOut = !signalLampRelayDigitalOut;
    buzzerRelayDigitalOut = !buzzerRelayDigitalOut;
}

void SignalLampController::start() {
    signalLampRelayDigitalOut = 1;
    buzzerRelayDigitalOut = 1;
    ticker.attach(mbed::callback(this, &SignalLampController::signalLampTickerHandler), signalLampInterval);
}

void SignalLampController::stop() {
    signalLampRelayDigitalOut = 0;
    buzzerRelayDigitalOut = 0;
    ticker.detach();
}

void SignalLampController::alarm() {
    signalLampRelayDigitalOut = 1;
    buzzerRelayDigitalOut = 1;
}