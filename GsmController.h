#ifndef _GSM_CONTROLLER_H_
#define _GSM_CONTROLLER_H_

#include "mbed.h"
#include "PinConfig.h"

#define READ_TIMEOUT 2s

class GsmController {
    public:
        GsmController(PinName sim800TXPin, PinName sim800RXPin): 
            sim800(sim800TXPin, sim800RXPin), 
            atCmdParser(&sim800, "\r\n"),
            sim800Thread(osPriorityNormal, 600)
        {
            sim800.set_baud(9600);
            sim800.set_format(
                /* bits */ 8,
                /* parity */ BufferedSerial::None,
                /* stop bit */ 1
            );
            atCmdParser.debug_on(true);
            sim800Thread.start(mbed::callback(this, &GsmController::sim800ThreadHandler));
        }
        bool initGsm();
        void setRingCallback(Callback<void()> callback);

    private:
        Thread sim800Thread;
        void sim800ThreadHandler();
        BufferedSerial sim800;
        ATCmdParser atCmdParser;
        bool canRead;
        void messageHandler(char* message);
        bool ring;
        Callback<void()> ringCallback;
};


#endif