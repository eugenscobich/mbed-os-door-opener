#include "mbed.h"
#include "GsmController.h"
#include "PinConfig.h"
#include <cstdio>
#include <cstring>

void GsmController::sim800ThreadHandler() {
    char buf[1] = {0};
    char message[64] = {0};
    while(1) {
        if (canRead) {
            uint32_t num = atCmdParser.read(buf, sizeof(buf));
            if (num > 0) {
                strncat(message, buf, sizeof(message) - 1);
                message[63] = '\0';
                if (buf[0] == '\n') {
                    messageHandler(message);
                    message[0] = '\0';
                }
            }
        }
    }
}

void GsmController::messageHandler(char * message) {
    printf("%s", message);
    char* found = strstr(message, "RING");
    if (found != NULL) {
        printf("Found: %s\n", found);
        phoneNumber[0] = '\0';
        ring = true;
    }

    if (ring) {
        found = strstr(message, "+CLIP: \"");
        if (found != NULL) {
            printf("Found: %s\n", found);
            found = strstr(message, "\",");
            if (found != NULL) {
                int endPosition = found - message;
                strncpy(phoneNumber, message + 8, endPosition - 8);
                phoneNumber[15] = '\0';
                printf("Phone number: %s\n", phoneNumber);
                printf("ringCallbackCalled: %d\n", ringCallbackCalled);
                if (ringCallback && ringCallbackCalled == false) {
                    ringCallbackCalled = true;
                    ringCallback.call();
                }
                ring = false;
            }
        }
    }
}

void GsmController::call(bool alarm) {
    canRead = false;
    if (strstr(phoneNumber, ALLOWED_PHONE_NUMBER_1) || strstr(phoneNumber, ALLOWED_PHONE_NUMBER_2)) {
        atCmdParser.send("ATD%s;", phoneNumber);
    } else {
        atCmdParser.send("ATD%s;", ALLOWED_PHONE_NUMBER_1);
    }
    if (alarm) {
        ThisThread::sleep_for(60s);
    } else {
        ThisThread::sleep_for(12s);
    }
    hangup();
    canRead = true;
}

char GsmController::waitForDFMTTone() {
    canRead = false;
    char ch;
    atCmdParser.recv("+DTMF: %c", &ch);
    canRead = true;
    return ch;
}

void GsmController::sendDTFMTone(char* number) {
    canRead = false;
    atCmdParser.send("AT+CLDTMF=5,\"%s\"", number);
    canRead = true;
}

void GsmController::hangup() {
    canRead = false;
    atCmdParser.send("ATH") && atCmdParser.recv("OK");
    ring = false;
    ringCallbackCalled = false;
    canRead = true;
}

void GsmController::answer() {
    canRead = false;
    atCmdParser.send("ATA") && atCmdParser.recv("OK");
    ring = false;
    ringCallbackCalled = false;
    canRead = true;
}

bool GsmController::isOK() {
    canRead = false;
    bool statusOk = atCmdParser.send("AT") && atCmdParser.recv("OK");
    canRead = true;
    return statusOk;
}

bool GsmController::initGsm() {
    canRead = false;
    bool statusOk = atCmdParser.send("AT") && atCmdParser.recv("OK");
    printf("Send Comand: %s, success: %d\r\n", "AT", statusOk);
    bool simOk = statusOk && atCmdParser.send("AT+CPIN?") && atCmdParser.recv("+CPIN: READY");
    printf("Send Comand: %s, success: %d\r\n", "AT+CPIN?", simOk);
    bool gsmOk = simOk && atCmdParser.send("AT+CREG?") && atCmdParser.recv("+CREG: 0,1");
    printf("Send Comand: %s, success: %d\r\n", "AT+CREG?", gsmOk);
    bool aonOk = gsmOk && atCmdParser.send("AT+CLIP=1") && atCmdParser.recv("OK");
    printf("Send Comand: %s, success: %d\r\n", "AT+CLIP=1", aonOk);
    bool dtmfOk = aonOk && atCmdParser.send("AT+DDET=1") && atCmdParser.recv("OK");
    printf("Send Comand: %s, success: %d\r\n", "AT+DDET=1", aonOk);
    
    canRead = true;
    return aonOk;
}

void GsmController::setRingCallback(Callback<void()> callback) {
    ringCallback = callback;
}