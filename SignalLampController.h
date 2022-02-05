#ifndef _SIGNAL_LAMP_CONTROLLER_H_
#define _SIGNAL_LAMP_CONTROLLER_H_

#include "mbed.h"

class SignalLampController {
public:
  SignalLampController(PinName signalLampRelayPin, PinName buzzerPin,
                       Kernel::Clock::duration_u32 interval = 1s)
      : signalLampRelayDigitalOut(signalLampRelayPin, 0),
        buzzerRelayDigitalOut(buzzerPin, 0), 
        signalLampInterval(interval) {
  };

  void start();
  void stop();
  void alarm();

private:
  DigitalOut signalLampRelayDigitalOut;
  DigitalOut buzzerRelayDigitalOut;

  Ticker ticker;
  void signalLampTickerHandler();
  Kernel::Clock::duration_u32 signalLampInterval;
};

#endif