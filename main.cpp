#include "mbed.h"
#include "DoorController.h"
#include "OledController.h"
#include "SignalLampController.h"
#include "PinConfig.h"

#define COMMAND_UNKNOWN 0
#define COMMAND_OPEN 1
#define COMMAND_CLOSE 2
#define COMMAND_STOP 3
#define COMMAND_OPEN_LEFT_DOOR 4

#define SIGNAL_LAMP_OFF_FLAG (1UL << 0)
#define SIGNAL_LAMP_ON_FLAG (1UL << 1)


DoorController leftDoorController(LEFT_OUT_A_PIN, LEFT_OUT_B_PIN, MOTOR_ACTUATOR_SWITCH_RELAY_PIN, LEFT_DOOR_PWM_PIN, LEFT_DOOR_CURRENT_PIN, LEFT_DOOR_OPEN_PIN, LEFT_DOOR_CLOSE_PIN, LEFT_MOTOR_COUNTER_PIN, 50);
DoorController rightDoorController(RIGHT_OUT_A_PIN, RIGHT_OUT_B_PIN, MOTOR_ACTUATOR_SWITCH_RELAY_PIN, RIGHT_DOOR_PWM_PIN, RIGHT_DOOR_CURRENT_PIN, RIGHT_DOOR_OPEN_PIN, RIGHT_DOOR_CLOSE_PIN, RIGHT_MOTOR_COUNTER_PIN, 50);

BufferedSerial sim800(SIM800_TX_PIN, SIM800_RX_PIN);
BufferedSerial pc(PC_TX_PIN, PC_RX_PIN);

OledController oled(PB_7, PB_6);

SignalLampController signalLampController(SIGNAL_LAMP_PIN, BUZZER_PIN);

Thread oledRefereshThread(osPriorityNormal, 700);
Thread sim800Thread(osPriorityNormal, 256);
Thread pcThread(osPriorityNormal, 256);

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

void sim800ThreadHandler() {
    char buf[32] = {0};
    while(1) {
        uint32_t num = sim800.read(buf, sizeof(buf));
        if (num > 0) {
            pc.write(buf, num);
        }
    }
}


void pcThreadHandler() {
    char buf[32] = {0};
    while(1) {
        uint32_t num = pc.read(buf, sizeof(buf));
        if (num > 0) {
            sim800.write(buf, num);
        }
    }
}

void sim800Callback()
{
    char c;
    // Read the data to clear the receive interrupt.
    if (sim800.read(&c, 1)) {
        // Echo the input back to the terminal.
        pc.write(&c, 1);
    }
}

void pcCallback()
{
    char c;
    // Read the data to clear the receive interrupt.
    if (pc.read(&c, 1)) {
        // Echo the input back to the terminal.
        sim800.write(&c, 1);
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
        oled.println("Both doors are stoped");
        currentCommand = COMMAND_UNKNOWN;
        signalLampController.stop();
    }
}

void openedDoorHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_OPENED && rightDoorController.getDoorState() == DOOR_STATE_OPENED) {
        oled.println("Both doors are opened");
        currentCommand = COMMAND_UNKNOWN;
        signalLampController.stop();
    }
}

void closedDoorHandler() {
    if (leftDoorController.getDoorState() == DOOR_STATE_CLOSED && rightDoorController.getDoorState() == DOOR_STATE_CLOSED) {
        oled.println("Both doors are closed");
        oled.println("Lock");
        leftDoorController.lock();
        rightDoorController.lock();
        ThisThread::sleep_for(500ms);
        signalLampController.stop();
        currentCommand = COMMAND_UNKNOWN;
        interiorLampSwithchOffTimeout.attach(interiorLampSwithchOffHandler, 30s);
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

// main() runs in its own thread in the OS
int main()
{

    leftDoorController.setOpenedCallback(mbed::callback(openedDoorHandler));
    leftDoorController.setClosedCallback(mbed::callback(closedDoorHandler));
    leftDoorController.setStopedCallback(mbed::callback(stopedDoorHandler));
    rightDoorController.setOpenedCallback(mbed::callback(openedDoorHandler));
    rightDoorController.setClosedCallback(mbed::callback(closedDoorHandler));
    rightDoorController.setStopedCallback(mbed::callback(stopedDoorHandler));

    leftDoorController.setOpenAnotherDoorCallback(mbed::callback(&rightDoorController, &DoorController::open));
    rightDoorController.setCloseAnotherDoorCallback(mbed::callback(&leftDoorController, &DoorController::close));

    commandButtonDebounceIn.fall(mbed::callback(commandButtonPressHandler));

    pc.set_baud(9600);
    sim800.set_baud(9600);

    char com[] = "AT\r\n";
    sim800.write(com, sizeof(com));

    sim800Thread.start(mbed::callback(sim800ThreadHandler));
    pcThread.start(mbed::callback(pcThreadHandler));
    
    printf("Hi !!!\r\n");
    oled.println("Hi !!!");

    oledRefereshThread.start(mbed::callback(oledRefereshThreadHandler));

    char printCommandBuff[8];
    char printLogBuff[22];
    while (true) {
        if (currentCommand != previousCommand && currentCommand != COMMAND_UNKNOWN) {
            printCommand(printCommandBuff);
            sprintf(printLogBuff, "Command: %s", printCommandBuff);
            oled.println(printLogBuff);

            if (currentCommand == COMMAND_OPEN) {
                signalLampController.start();
                interiorLampDigitalOut = 1;
                leftDoorController.unlock();
                rightDoorController.unlock();
                ThisThread::sleep_for(3500ms);
                leftDoorController.open();
            } else if (currentCommand == COMMAND_CLOSE) {
                signalLampController.start();
                leftDoorController.unlock();
                rightDoorController.unlock();
                ThisThread::sleep_for(3500ms);
                rightDoorController.close();
            } else if (currentCommand == COMMAND_STOP) {
                leftDoorController.stop();
                rightDoorController.stop();
            } else if (currentCommand == COMMAND_OPEN_LEFT_DOOR) {
                signalLampController.start();
                leftDoorController.unlock();
                ThisThread::sleep_for(500ms);
                leftDoorController.open();
            }

            previousCommand = currentCommand;
        }

        ThisThread::sleep_for(1ms);
    }
}

