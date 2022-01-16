#include "mbed.h"
#include "DoorController.h"
#include "GsmController.h"
#include "OledController.h"
#include "SignalLampController.h"
#include "PinConfig.h"

#define COMMAND_UNKNOWN 0
#define COMMAND_OPEN 1
#define COMMAND_CLOSE 2
#define COMMAND_STOP 3
#define COMMAND_OPEN_LEFT_DOOR 4


OledController oled(PB_7, PB_6);
SignalLampController signalLampController(SIGNAL_LAMP_PIN, BUZZER_PIN);

DoorController leftDoorController(LEFT_OUT_A_PIN, LEFT_OUT_B_PIN, MOTOR_ACTUATOR_SWITCH_RELAY_PIN, LEFT_DOOR_PWM_PIN, LEFT_DOOR_CURRENT_PIN, LEFT_DOOR_OPEN_PIN, LEFT_DOOR_CLOSE_PIN, LEFT_MOTOR_COUNTER_PIN, 50);
DoorController rightDoorController(RIGHT_OUT_A_PIN, RIGHT_OUT_B_PIN, MOTOR_ACTUATOR_SWITCH_RELAY_PIN, RIGHT_DOOR_PWM_PIN, RIGHT_DOOR_CURRENT_PIN, RIGHT_DOOR_OPEN_PIN, RIGHT_DOOR_CLOSE_PIN, RIGHT_MOTOR_COUNTER_PIN, 50);
GsmController gsm(SIM800_TX_PIN, SIM800_RX_PIN);

Thread oledRefereshThread(osPriorityNormal, 700);

DebounceIn commandButtonDebounceIn(BUTTON_PIN, PullUp);
DigitalOut interiorLampDigitalOut(INTERIOR_LAMP_RELAY_PIN, 0);

int previousCommand = COMMAND_UNKNOWN;
int currentCommand = COMMAND_UNKNOWN;
int opositeCommand = COMMAND_OPEN;

Timeout interiorLampSwithchOffTimeout;
char printDoorStateBuff[9] = {0};

void oledRefereshThreadHandler() {
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
    }
}


void interiorLampSwithchOffHandler() {
    interiorLampDigitalOut = 0;
}

void stopedDoorHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_STOPED && rightDoorController.getDoorState() == DOOR_STATE_STOPED) {
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

void closedDoorHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_CLOSED && rightDoorController.getDoorState() == DOOR_STATE_CLOSED) {
        printf("Both doors are closed\n");
        oled.println("Both doors are closed");
        oled.println("Lock");
        leftDoorController.lock();
        rightDoorController.lock();
        ThisThread::sleep_for(3500ms);
        printf("Both doors are locked\n");
        oled.println("Both doors are locked");
        signalLampController.stop();
        currentCommand = COMMAND_UNKNOWN;
        interiorLampSwithchOffTimeout.attach(interiorLampSwithchOffHandler, 30s);
        gsm.callBack();
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

        char gsmCommand = gsm.waitForDFMTTone();

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
        } else {
            commandButtonPressHandler();     
        }

        ThisThread::sleep_for(1s);
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

        ThisThread::sleep_for(2s);
        gsm.hangup();
        
    }
}

void initGsm() {
    gsm.setRingCallback(mbed::callback(ringHandler));
    
    printf("Wait 30s GSM to start\r\n");
    oled.println("Wait 30s GSM to start");
    ThisThread::sleep_for(30s);
    printf("Init GSM\r\n");
    oled.println("Init GSM");
    bool successfulGsm = gsm.initGsm();
    if (!successfulGsm) {
        printf("Init GSM FAILED\r\n");
        oled.println("Init GSM FAILED");
        signalLampController.alarm();
    } else {
        printf("Init GSM OK\r\n");
        oled.println("Init GSM OK");
    }

}

// main() runs in its own thread in the OS
int main()
{

    leftDoorController.setOpenedCallback(mbed::callback(openedDoorHandler));
    leftDoorController.setClosedCallback(mbed::callback(closedDoorHandler));
    leftDoorController.setStopedCallback(mbed::callback(stopedDoorHandler));
    rightDoorController.setOpenedCallback(mbed::callback(openedDoorHandler));
    rightDoorController.setClosedCallback(mbed::callback(closedDoorHandler));
    rightDoorController.setStopedCallback(mbed::callback(stopedDoorHandler));


    commandButtonDebounceIn.fall(mbed::callback(commandButtonPressHandler));

    oledRefereshThread.start(mbed::callback(oledRefereshThreadHandler));

    printf("Hi !!!\r\n");
    oled.println("Hi !!!");

    initGsm();

    Watchdog &watchdog = Watchdog::get_instance();
    watchdog.start();

    char printCommandBuff[8];
    char printLogBuff[22];
    while (true) {
        if (currentCommand != previousCommand && currentCommand != COMMAND_UNKNOWN) {
            printCommand(printCommandBuff);
            sprintf(printLogBuff, "Command: %s", printCommandBuff);
            oled.println(printLogBuff);

            if (currentCommand == COMMAND_OPEN) {
                leftDoorController.setOpenAnotherDoorCallback(mbed::callback(&rightDoorController, &DoorController::open));
                rightDoorController.setCloseAnotherDoorCallback(NULL);
                signalLampController.start();
                interiorLampDigitalOut = 1;
                leftDoorController.unlock();
                rightDoorController.unlock();
                ThisThread::sleep_for(ACTUATOR_LOCK_UNLOCK_TIME_WITH_TIMEOUT);
                leftDoorController.open();
            } else if (currentCommand == COMMAND_CLOSE) {
                leftDoorController.setOpenAnotherDoorCallback(NULL);
                rightDoorController.setCloseAnotherDoorCallback(mbed::callback(&leftDoorController, &DoorController::close));
                signalLampController.start();
                leftDoorController.unlock();
                rightDoorController.unlock();
                ThisThread::sleep_for(ACTUATOR_LOCK_UNLOCK_TIME_WITH_TIMEOUT);
                rightDoorController.close();
            } else if (currentCommand == COMMAND_STOP) {
                leftDoorController.setOpenAnotherDoorCallback(NULL);
                rightDoorController.setCloseAnotherDoorCallback(NULL);
                leftDoorController.stop();
                rightDoorController.stop();
            } else if (currentCommand == COMMAND_OPEN_LEFT_DOOR) {
                leftDoorController.setOpenAnotherDoorCallback(NULL);
                rightDoorController.setCloseAnotherDoorCallback(NULL);
                signalLampController.start();
                leftDoorController.unlock();
                ThisThread::sleep_for(ACTUATOR_LOCK_UNLOCK_TIME_WITH_TIMEOUT);
                leftDoorController.open();
            }

            previousCommand = currentCommand;
        }
        watchdog.kick();
        ThisThread::sleep_for(10ms);
    }
}

