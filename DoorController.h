
#ifndef _DOOR_CONTROLLER_H_
#define _DOOR_CONTROLLER_H_

#include "mbed.h"
#include "DebounceIn.h"

// Door States
#define DOOR_STATE_UNKNOWN  0
#define DOOR_STATE_OPEN     1
#define DOOR_STATE_CLOSE    2
#define DOOR_STATE_OPENED   3
#define DOOR_STATE_CLOSED   4
#define DOOR_STATE_OPENING  5
#define DOOR_STATE_CLOSING  6
#define DOOR_STATE_LOCK     7
#define DOOR_STATE_UNLOCK   8
#define DOOR_STATE_LOCKED   9
#define DOOR_STATE_UNLOCKED 10
#define DOOR_STATE_STOP     11
#define DOOR_STATE_STOPED   12
#define DOOR_STATE_STOPING  13
#define DOOR_STATE_ALARM    14

#define ACTION_DELAY_TIME 1000ms
#define CHANGE_RELAY_STATE_TIME 300ms
#define PWM_CHANGE_SPEED_TIME 70ms
#define ACTUATOR_LOCK_UNLOCK_TIME 500ms
#define COUNTER_DELTA 10

class DoorController {
public:
  DoorController(PinName openRelayPin, PinName closeRelayPin,
                 DigitalOut &motorActuatorRelayDigitalOut, PinName pwmOutPin,
                 PinName currentSensorPin, PinName openSignalPin,
                 PinName closeSignalPin, PinName counterPin, uint16_t doorMaxCount = 300, uint16_t currentTrashhold = 2000)
      : openRelayDigitalOut(openRelayPin),
        closeRelayDigitalOut(closeRelayPin),
        pwmOut(pwmOutPin),
        motorActuatorRelayDigitalOut(motorActuatorRelayDigitalOut),
        currentSensorAnalogIn(currentSensorPin),
        openSignalDebounceIn(openSignalPin, PullNone, 1ms, 10),
        closeSignalDebounceIn(closeSignalPin, PullNone, 1ms, 10),
        counterDebounceIn(counterPin, PullNone, 2ms, 10),
        maxCount(doorMaxCount),
        currentTrashhold(currentTrashhold)
        {
             counterDebounceIn.fall(mbed::callback(this, &DoorController::counterIntrerruptHandler));
             openSignalDebounceIn.fall(mbed::callback(this, &DoorController::openSignalIntrerruptHandler));
             closeSignalDebounceIn.fall(mbed::callback(this, &DoorController::closeSignalIntrerruptHandler));
             pwmOut.period_us(1000);
             pwmOut.write(1.0);

            if (isDoorOpened()) {
                currentDoorState = DOOR_STATE_OPENED;
                openedCallbackCalled = true;
            }

            if (isDoorClosed()) {
                currentDoorState = DOOR_STATE_CLOSED;
                closedCallbackCalled = true;
            }

            currentReadTicker.attach(mbed::callback(this, &DoorController::currentReadTickerHandler), 100ms);

        };

  void openBothDoors(Callback<void()> callback);
  void closeBothDoors(Callback<void()> callback);
  void open();
  void close();
  void stop();
  void lock();
  void unlock();
  uint16_t getCount();
  uint16_t getCurrent();
  uint8_t getDoorState();
  void printDoorState(char* buff);
  void setOpenAnotherDoorCallback(Callback<void()> callback);
  void setCloseAnotherDoorCallback(Callback<void()> callback);
  void setLockAnotherDoorCallback(Callback<void()> callback);
  void setUnlockAnotherDoorCallback(Callback<void()> callback);
  void setOpenedCallback(Callback<void()> callback);
  void setClosedCallback(Callback<void()> callback);
  void setStopedCallback(Callback<void()> callback);
  void setAlarmCallback(Callback<void()> callback);
  void handle();


  bool isDoorOpened();
  bool isDoorClosed();
  bool isDoorOpening();
  bool isDoorClosing();

private:
  uint16_t maxCount;
  DigitalOut openRelayDigitalOut;
  DigitalOut closeRelayDigitalOut;

  DigitalOut &motorActuatorRelayDigitalOut;

  PwmOut pwmOut;
  AnalogIn currentSensorAnalogIn;

  DebounceIn openSignalDebounceIn;
  DebounceIn closeSignalDebounceIn;

  DebounceIn counterDebounceIn;

  uint8_t previousDoorState;
  uint8_t currentDoorState;

  uint16_t count;
  
  void counterIntrerruptHandler();
  void openSignalIntrerruptHandler();
  void closeSignalIntrerruptHandler();

  Callback<void()> openAnotherDoorCallback;
  bool openAnotherDoorCallbackCalled;
  Callback<void()> closeAnotherDoorCallback;
  bool closeAnotherDoorCallbackCalled;

  Callback<void()> lockAnotherDoorCallback;
  Callback<void()> unlockAnotherDoorCallback;
  
  Callback<void()> stopedCallback;
  bool stopedCallbackCalled;
  Callback<void()> openedCallback;
  bool openedCallbackCalled;
  Callback<void()> closedCallback;
  bool closedCallbackCalled;
  Callback<void()> alarmCallback;
  bool alarmCallbackCalled;
  bool armed;


  //Timeout timeout;
  void lockDelayed();
  void startLockTimeoutHandler();
  void endLockTimeoutHandler();
  void unlockDelayed();
  void startUnlockTimeoutHandler();
  void endUnlockTimeoutHandler();

    
  Ticker currentReadTicker;
  void currentReadTickerHandler();
  uint16_t currents[10];
  uint8_t currentsArrayPointer;
  uint16_t currentTrashhold;
  bool readCurrent;
  
  Ticker speedTicker;
  void speedUpTickerHandler();
  void speedUpTimeoutHandler();
  bool fullStopFlag;
  void speedDownTickerHandler();

  void handleStopSignals();
  void handleCommand();
  void handleCounter();
  void handleCurrent();

  void startOpening();
  void startClosing();
  void startStoping(bool fullStop = true);

 void stopReadCurrent();

  void stopAll();
};

#endif