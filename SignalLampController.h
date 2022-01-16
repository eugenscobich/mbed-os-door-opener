#ifndef _SIGNAL_LAMP_CONTROLLER_H_
#define _SIGNAL_LAMP_CONTROLLER_H_

#include "mbed.h"

class SignalLampController {
public:
  SignalLampController(PinName signalLampRelayPin, PinName buzzerPin,
                       Kernel::Clock::duration_u32 interval = 1s)
      : signalLampRelayDigitalOut(signalLampRelayPin, 0),
        buzzerRelayDigitalOut(buzzerPin, 0), signalLampInterval(interval),
        signalLampThread(osPriorityNormal, 256) {
    signalLampThread.start(
        mbed::callback(this, &SignalLampController::signalLampThreadHandler));
  };

  void start();
  void stop();
  void alarm();

private:
  DigitalOut signalLampRelayDigitalOut;
  DigitalOut buzzerRelayDigitalOut;

  Thread signalLampThread;
  void signalLampThreadHandler();
  bool isSignalLampOn;
  Kernel::Clock::duration_u32 signalLampInterval;
};

#endif