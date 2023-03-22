#include "mbed.h"
#include "DoorController.h"
#include "GsmController.h"
#include "OledController.h"
#include "SignalLampController.h"
#include "PinConfig.h"
#include <cstdint>

#define COMMAND_UNKNOWN 0
#define COMMAND_OPEN 1
#define COMMAND_CLOSE 2
#define COMMAND_STOP 3
#define COMMAND_OPEN_LEFT_DOOR 4

#define VERSION "v2.0.1"

#define LEFT_DOOR_MAX_COUNT 210
#define RIGHT_DOOR_MAX_COUNT 210

OledController oled(PB_7, PB_6);
SignalLampController signalLampController(SIGNAL_LAMP_PIN, BUZZER_PIN);

DigitalOut motorActuatorSwitchRelayDigitalOut(MOTOR_ACTUATOR_SWITCH_RELAY_PIN);

DoorController leftDoorController(LEFT_OUT_A_PIN, LEFT_OUT_B_PIN, motorActuatorSwitchRelayDigitalOut, 
    LEFT_DOOR_PWM_PIN, LEFT_DOOR_CURRENT_PIN, LEFT_DOOR_OPEN_PIN, LEFT_DOOR_CLOSE_PIN, LEFT_MOTOR_COUNTER_PIN, 
    LEFT_DOOR_MAX_COUNT);
DoorController rightDoorController(RIGHT_OUT_A_PIN, RIGHT_OUT_B_PIN, motorActuatorSwitchRelayDigitalOut, 
    RIGHT_DOOR_PWM_PIN, RIGHT_DOOR_CURRENT_PIN, RIGHT_DOOR_OPEN_PIN, RIGHT_DOOR_CLOSE_PIN, RIGHT_MOTOR_COUNTER_PIN, 
    RIGHT_DOOR_MAX_COUNT);

GsmController gsm(SIM800_TX_PIN, SIM800_RX_PIN);

Thread oledRefereshThread(osPriorityBelowNormal, 700);

DebounceIn commandButtonDebounceIn(BUTTON_PIN, PullUp, 1ms, 5);
DigitalOut interiorLampDigitalOut(INTERIOR_LAMP_RELAY_PIN, 0);

uint8_t previousCommand = COMMAND_UNKNOWN;
uint8_t currentCommand = COMMAND_UNKNOWN;
uint8_t opositeCommand = COMMAND_OPEN;

Timeout interiorLampSwithchOffTimeout;

DebounceIn rxD0DebounceIn(RX480E_4_D0_PIN, PullDown);
DebounceIn rxD1DebounceIn(RX480E_4_D1_PIN, PullDown);
//DebounceIn rxD2DebounceIn(RX480E_4_D2_PIN, PullDown);
DebounceIn rxD3DebounceIn(RX480E_4_D3_PIN, PullDown);

//bool ac220signals[100];
//uint8_t ac220signalsPointer;
//DigitalIn ac220signalsDigitalIn(CHECK_CURRENT_PIN, PullUp);
bool gsmIsMissing = true;
uint8_t gsmInitCount = 0;
bool readBtn = true;
bool canOpenByRemote = true;

void oledRefereshThreadHandler() {
    char printDoorStateBuff[9] = {0};
    while(1) {
        leftDoorController.printDoorState(printDoorStateBuff);
        oled.printLeftDoorDetails(leftDoorController.getCount(), leftDoorController.getCurrent(), printDoorStateBuff);

        rightDoorController.printDoorState(printDoorStateBuff);
        oled.printRightDoorDetails(rightDoorController.getCount(), rightDoorController.getCurrent(), printDoorStateBuff);

        oled.displayLines();
        ThisThread::sleep_for(100ms);
    }
}


void commandButtonPressHandler() {
    if (readBtn) {
        if (currentCommand == COMMAND_UNKNOWN) {
            currentCommand = opositeCommand;
            if (opositeCommand == COMMAND_OPEN) {
                opositeCommand = COMMAND_CLOSE;
            } else if (opositeCommand == COMMAND_CLOSE) {
                opositeCommand = COMMAND_OPEN;
            }
        } else if (currentCommand == COMMAND_OPEN) {
            currentCommand = COMMAND_STOP;
            opositeCommand = COMMAND_CLOSE;
        } else if (currentCommand == COMMAND_CLOSE) {
            currentCommand = COMMAND_STOP;
            opositeCommand = COMMAND_OPEN;
        } else if (currentCommand == COMMAND_STOP) {
            currentCommand = opositeCommand;
        } else if (currentCommand == COMMAND_OPEN_LEFT_DOOR) {
            currentCommand = COMMAND_STOP;
            opositeCommand = COMMAND_CLOSE;
        }
        readBtn = false;
    }
    
}


void interiorLampSwithchOffHandler() {
    interiorLampDigitalOut = 0;
}

void stopedDoorHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_STOPED && (rightDoorController.getDoorState() == DOOR_STATE_STOPED || rightDoorController.getDoorState() == DOOR_STATE_CLOSED)) {
        printf("Both doors are stoped\n");
        oled.println("Both doors are stoped");
        currentCommand = COMMAND_UNKNOWN;
        signalLampController.stop();
    }
}

void openedDoorHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_OPENED && rightDoorController.getDoorState() == DOOR_STATE_OPENED) {
        printf("Both doors are opened\n");
        oled.println("Both doors are opened");
        currentCommand = COMMAND_UNKNOWN;
        signalLampController.stop();
    }
    if (currentCommand == COMMAND_OPEN_LEFT_DOOR && leftDoorController.getDoorState() == DOOR_STATE_OPENED) {
        printf("Left doors is opened\n");
        oled.println("Left doors is opened");
        currentCommand = COMMAND_UNKNOWN;
        signalLampController.stop();
    
    }
}

void closedDoorEndHandler() {
    //printf("Both doors are locked\n");
    //oled.println("Both doors are locked");
    signalLampController.stop();
    
    interiorLampSwithchOffTimeout.attach(interiorLampSwithchOffHandler, 30s);
    if (gsmIsMissing == false) {
        printf("Call\n");
        oled.println("Call");
        Watchdog::get_instance().kick();
        gsm.call();
        Watchdog::get_instance().kick();
        printf("Finish Call\n");
        oled.println("Finish Call");
    }
    currentCommand = COMMAND_UNKNOWN;
}

void closedDoorStartHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_CLOSED && 
    (rightDoorController.getDoorState() == DOOR_STATE_CLOSED || rightDoorController.getDoorState() == DOOR_STATE_LOCKED || rightDoorController.getDoorState() == DOOR_STATE_STOPED)) {
        printf("Both doors are closed\n");
        oled.println("Both doors are closed");
        //oled.println("Lock");

        //rightDoorController.setLockAnotherDoorCallback(mbed::callback(&leftDoorController, &DoorController::lock));
        //leftDoorController.setLockAnotherDoorCallback(mbed::callback(closedDoorEndHandler));
        //rightDoorController.lock();
        closedDoorEndHandler();
    }
}

void printCommand(char* buff)
{
    if (currentCommand == COMMAND_UNKNOWN) {
        strncpy(buff, "Unknown", 8);
    } else if (currentCommand == COMMAND_OPEN) {
        strncpy(buff, "Open", 8);
    } else if (currentCommand == COMMAND_CLOSE) {
        strncpy(buff, "Close", 8);
    } else if (currentCommand == COMMAND_STOP) {
        strncpy(buff, "Stop", 8);
    } else if (currentCommand == COMMAND_OPEN_LEFT_DOOR) {
        strncpy(buff, "Open LD", 8);
    }
}

void ringHandler() {
    if (strstr(gsm.phoneNumber, ALLOWED_PHONE_NUMBER_1) || strstr(gsm.phoneNumber, ALLOWED_PHONE_NUMBER_2)) {
        printf("Call received: %s\n", gsm.phoneNumber);
        oled.println("Call received");
        oled.println(gsm.phoneNumber);
         
        gsm.answer();
        Watchdog::get_instance().kick();
        char gsmCommand = gsm.waitForDFMTTone();
        Watchdog::get_instance().kick();
        bool readStatus = false;
        if (gsmCommand == '1') {
            currentCommand = COMMAND_OPEN;
            opositeCommand = COMMAND_CLOSE;
        } else if (gsmCommand == '2') {
            currentCommand = COMMAND_CLOSE;
            opositeCommand = COMMAND_OPEN;
        } else if (gsmCommand == '3') {
            currentCommand = COMMAND_STOP;
            if (opositeCommand == COMMAND_OPEN) {
                opositeCommand = COMMAND_CLOSE;
            } else if (opositeCommand == COMMAND_CLOSE) {
                opositeCommand = COMMAND_OPEN;
            }
        } else if (gsmCommand == '4') {
            currentCommand = COMMAND_OPEN_LEFT_DOOR;
            opositeCommand = COMMAND_CLOSE;
        } else if (gsmCommand == '5') {
            readStatus = true;
        } else {
            printf("No Command\n");
            oled.println("No command");
            //commandButtonPressHandler();     
        }

        ThisThread::sleep_for(1s);
        if (readStatus) {
            gsm.sendDTFMTone((char*)DTMF_TONE);
            if (!leftDoorController.isDoorClosed() || !rightDoorController.isDoorClosed()) {
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
            }
        } else {
            if (currentCommand == COMMAND_OPEN) {
                gsm.sendDTFMTone((char*)DTMF_TONE);
            } else if (currentCommand == COMMAND_CLOSE) {
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
            } else if (currentCommand == COMMAND_STOP) {
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
            } else if (currentCommand == COMMAND_OPEN_LEFT_DOOR) {
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
                ThisThread::sleep_for(1s);
                gsm.sendDTFMTone((char*)DTMF_TONE);
            }
        }
        Watchdog::get_instance().kick();
        ThisThread::sleep_for(2s);
        gsm.hangup();
        Watchdog::get_instance().kick();
        
    }
}

void initGsm() {
    gsm.setRingCallback(mbed::callback(ringHandler));

    ThisThread::sleep_for(10s);
    Watchdog::get_instance().kick();
    if(gsm.isOK()) {
        if (gsmInitCount == 0) {
            printf("Wait 30s GSM to start\r\n");
            oled.println("Wait 30s GSM to start");
            ThisThread::sleep_for(15s);
            Watchdog::get_instance().kick();
            ThisThread::sleep_for(15s);
            Watchdog::get_instance().kick();
        } else {
            printf("Second start GSM\r\n");
            oled.println("Second start GSM");
        }
        printf("Init GSM\r\n");
        oled.println("Init GSM");
        bool successfulGsm = gsm.initGsm();
        if (!successfulGsm) {
            printf("Init GSM FAILED\r\n");
            oled.println("Init GSM FAILED");
            gsmInitCount++;
            if (gsmInitCount < 3) {
                initGsm();
            } else {
                signalLampController.alarm();
            }
        } else {
            printf("Init GSM OK\r\n");
            oled.println("Init GSM OK");
            signalLampController.alarm();
            ThisThread::sleep_for(1s);
            signalLampController.stop();
            gsmIsMissing = false;
            canOpenByRemote = false;
        }
    } else {
        gsmIsMissing = true;
        printf("GSM is missing\r\n");
        oled.println("GSM is missing");
        signalLampController.alarm();
        ThisThread::sleep_for(10s);
        Watchdog::get_instance().kick();
        signalLampController.stop();
        gsmInitCount++;
        if (gsmInitCount < 3) {
            initGsm();
        } else {
            signalLampController.alarm();
        }
    }
}

void rxD0DebounceInHandler() {
    if (currentCommand != COMMAND_CLOSE) {
        currentCommand = COMMAND_CLOSE;
        opositeCommand = COMMAND_OPEN;
    } else {
        currentCommand = COMMAND_STOP;
    }
}

void rxD1DebounceInHandler() {
    if (canOpenByRemote) {
        if (currentCommand != COMMAND_OPEN) {
            currentCommand = COMMAND_OPEN;
            opositeCommand = COMMAND_CLOSE;
        } else {
            currentCommand = COMMAND_STOP;
        }
    }
}

void rxD2DebounceInHandler() {
    commandButtonPressHandler();
}

void rxD3DebounceInHandler() {
    if (currentCommand != COMMAND_OPEN_LEFT_DOOR) {
        currentCommand = COMMAND_OPEN_LEFT_DOOR;
        opositeCommand = COMMAND_CLOSE;
    } else {
        currentCommand = COMMAND_STOP;
    }
}

void alarmDoorHandler() {
    printf("Alarm !!!\r\n");
    oled.println("Alarm !!!");
    if (gsmIsMissing == false) {        
        //gsm.call(true);
    }
    leftDoorController.stop();
    rightDoorController.stop();
}
/*
void ac220signalsHandler() {
    ac220signals[ac220signalsPointer++] = ac220signalsDigitalIn.read();
    if (ac220signalsPointer == 100) {
        ac220signalsPointer = 0;
    }

    uint32_t i = 0;
    for (uint8_t j = 0; j < 100; j++) {
        i = i + ac220signals[j];
    }
    if (i == 100) {
        printf("No 220 !!!\r\n");
        oled.println("No 220 !!!");
        alarmDoorHandler();
    }
}
*/

void handleDoors() {
    leftDoorController.handle();
    rightDoorController.handle();
}


void handleCommands() {
    char printCommandBuff[8];
    char printLogBuff[22];
    if (currentCommand != previousCommand && currentCommand != COMMAND_UNKNOWN) {
        printCommand(printCommandBuff);
        sprintf(printLogBuff, "Command: %s", printCommandBuff);
        oled.println(printLogBuff);

        if (currentCommand == COMMAND_OPEN) {
            signalLampController.start();
            interiorLampDigitalOut = 1;
            leftDoorController.setOpenAnotherDoorCallback(mbed::callback(&rightDoorController, &DoorController::open));
            rightDoorController.setCloseAnotherDoorCallback(NULL);
            leftDoorController.open();
        } else if (currentCommand == COMMAND_CLOSE) {
            signalLampController.start();
            interiorLampDigitalOut = 1;
            rightDoorController.setCloseAnotherDoorCallback(mbed::callback(&leftDoorController, &DoorController::close));
            leftDoorController.setOpenAnotherDoorCallback(NULL);
            rightDoorController.close();
        } else if (currentCommand == COMMAND_STOP) {
            leftDoorController.setOpenAnotherDoorCallback(NULL);
            rightDoorController.setCloseAnotherDoorCallback(NULL);
            if (previousCommand != COMMAND_OPEN_LEFT_DOOR) {
                leftDoorController.stop();
                rightDoorController.stop();
            } else {
                leftDoorController.stop();
            }
        } else if (currentCommand == COMMAND_OPEN_LEFT_DOOR) {
            signalLampController.start();
            interiorLampDigitalOut = 1;
            leftDoorController.setOpenAnotherDoorCallback(NULL);
            leftDoorController.open();
        }

        previousCommand = currentCommand;
        readBtn = true;
    }

}


// main() runs in its own thread in the OS
int main() {
    
    Watchdog::get_instance().start();
    commandButtonDebounceIn.fall(mbed::callback(commandButtonPressHandler));

    rxD0DebounceIn.fall(mbed::callback(rxD0DebounceInHandler));
    rxD1DebounceIn.fall(mbed::callback(rxD1DebounceInHandler));
    //rxD2DebounceIn.fall(mbed::callback(rxD2DebounceInHandler));
    rxD3DebounceIn.fall(mbed::callback(rxD3DebounceInHandler));

    leftDoorController.setOpenedCallback(mbed::callback(openedDoorHandler));
    leftDoorController.setClosedCallback(mbed::callback(closedDoorStartHandler));
    leftDoorController.setStopedCallback(mbed::callback(stopedDoorHandler));
    leftDoorController.setAlarmCallback(mbed::callback(alarmDoorHandler));
    rightDoorController.setOpenedCallback(mbed::callback(openedDoorHandler));
    rightDoorController.setClosedCallback(mbed::callback(closedDoorStartHandler));
    rightDoorController.setStopedCallback(mbed::callback(stopedDoorHandler));
    rightDoorController.setAlarmCallback(mbed::callback(alarmDoorHandler));

    oledRefereshThread.start(mbed::callback(oledRefereshThreadHandler));

    printf("Hi !!!\r\n");
    oled.println("Hi !!!");
    
    printf("%s\r\n", VERSION);
    oled.println(VERSION);

    initGsm();

    


    while (true) {
        handleCommands();
        //ac220signalsHandler();
        handleDoors();
        Watchdog::get_instance().kick();
        ThisThread::sleep_for(10ms);
    }
}

