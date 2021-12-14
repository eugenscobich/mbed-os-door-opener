#include "mbed.h"
#include "Adafruit_SSD1306.h"

PwmOut      LEFT_DOOR_PWM_PIN(PA_6);
PwmOut      RIGHT_DOOR_PWM_PIN(PA_7);

AnalogIn    LEFT_DOOR_CURRENT_PIN(PA_1);
AnalogIn    RIGHT_DOOR_CURRENT_PIN(PA_0);

DigitalIn LEFT_DOOR_OPEN_PIN(PB_12, PullUp);
DigitalIn LEFT_DOOR_CLOSE_PIN(PB_13, PullUp);
DigitalIn RIGHT_DOOR_OPEN_PIN(PB_14, PullUp);
DigitalIn RIGHT_DOOR_CLOSE_PIN(PB_15, PullUp);

DigitalIn CHECK_CURRENT_PIN(PA_8, PullUp);

BufferedSerial sim800(PA_9, PA_10);

DigitalIn RX480E_4_D0_PIN(PA_11, PullUp);
DigitalIn RX480E_4_D1_PIN(PA_12, PullUp);
DigitalIn RX480E_4_D2_PIN(PA_15, PullUp);
DigitalIn RX480E_4_D3_PIN(PB_3, PullUp);

DigitalOut OLED_5V(PB_5, 1);
DigitalOut OLED_GND(PB_4, 0);


I2C gI2C(PB_7, PB_6);
Adafruit_SSD1306_I2c gOled2(gI2C, D4, SSD_I2C_ADDRESS, 64, 128);


// main() runs in its own thread in the OS
int main()
{
    rtc_init();
    set_time(1256729737);

    


    printf("Hi 1 !!!\r\n");
    gOled2.clearDisplay();
    
    gOled2.printf("%ux%u OLED Display\r\n", gOled2.width(), gOled2.height());
    gOled2.display();

    printf("Hi 2 !!!\r\n");
    while (true) {
        time_t seconds = time(NULL);
        printf("Time as seconds since January 1, 1970 = %d\n", seconds);
        
        printf("Time as a basic string = %s", ctime(&seconds));
 
        char buffer[32];
        strftime(buffer, 32, "%I:%M %p\n", localtime(&seconds));
        printf("Time as a custom formatted string = %s", buffer);
        

        gOled2.printf("%s", ctime(&seconds));
        gOled2.display();

        ThisThread::sleep_for(1s);


    }
}

