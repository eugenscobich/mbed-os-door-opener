#ifndef _GSM_CONTROLLER_H_
#define _GSM_CONTROLLER_H_

#include "mbed.h"
#include "PinConfig.h"

#define ALLOWED_PHONE_NUMBER_1 "79849507"
#define ALLOWED_PHONE_NUMBER_2 "79753884"
#define READ_TIMEOUT 2s
#define DTMF_TONE "*"

class GsmController {
    public:
        GsmController(PinName sim800TXPin, PinName sim800RXPin): 
            sim800(sim800TXPin, sim800RXPin), 
            atCmdParser(&sim800, "\r\n"),
            sim800Thread(osPriorityNormal, 1024)
        {
            sim800.set_baud(9600);
            sim800.set_format(
                /* bits */ 8,
                /* parity */ BufferedSerial::None,
                /* stop bit */ 1
            );
            atCmdParser.debug_on(true);
            atCmdParser.set_timeout(5000);
            sim800Thread.start(mbed::callback(this, &GsmController::sim800ThreadHandler));
        }
        bool initGsm();
        void answer();
        void hangup();
        void call(bool alarm = false);
        void sendDTFMTone(char* number);
        char waitForDFMTTone();
        
        void setRingCallback(Callback<void()> callback);
        char phoneNumber[16] = {0};

    private:
        Thread sim800Thread;
        void sim800ThreadHandler();
        BufferedSerial sim800;
        ATCmdParser atCmdParser;
        bool canRead;
        void messageHandler(char* message);
        bool ring;
        Callback<void()> ringCallback;
        bool ringCallbackCalled;
        Timer timer;
};


#endif