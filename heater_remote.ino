#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/****************************************************************************/
/* Local Defines */
/****************************************************************************/
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

/* declaration for SSD1306 display connected using software SPI (default case) */
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

/* pins, LEDs and timeouts */
#define DURATION_MAX    30
#define DURATION_MIN    5
#define DURATION_PIN    A0
#define BUTTON          3
#define LED_R           4
#define LED_G           5

/* loop timeouts */
#define DISPLAY_LOOP_TIMEOUT    100
#define LED_LOOP_TIMEOUT        1000
#define BUTTON_LOOP_TIMEOUT     2000


/****************************************************************************/
/* Locals */
/****************************************************************************/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

typedef enum {
    ERR_OK,
    ERR_ERR
} ERR_STATUS;

typedef enum {
    STATE_INIT,
    STATE_STANDBY,
    STATE_PRE_HEAT,
    STATE_HEAT,
    STATE_ERR,
} STATE;

STATE mState;
bool mStateLedGreen;
bool mStateLedRed;
volatile bool mFlgButtonPressed;


/****************************************************************************/
/* Local Prototypes */
/****************************************************************************/
void static errorStateSet(void);

ERR_STATUS static peripheriInit(void);

void static displayRefresh(uint16_t duration);

void static ledRefresh(void);

void static durationGet(uint16_t *pDuration);

void static irqButton(void);

void setup(void);

void loop(void);


/****************************************************************************/
/** Function
 */
void static errorStateSet(void) {
    mState = STATE_ERR;
}


/****************************************************************************/
/** Function
 */
ERR_STATUS static peripheriInit(void) {
    /* init oled display */
    display.begin(SSD1306_SWITCHCAPVCC);
    display.display();

    /* init leds */
    pinMode(LED_G, OUTPUT);
    pinMode(LED_R, OUTPUT);

    /* init button */
    pinMode(BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON), irqButton, LOW);

    mState = STATE_STANDBY;

    return ERR_OK;
}


/****************************************************************************/
/** Function
 */
void static displayRefresh(uint16_t duration) {    
    /* refresh display */
    display.clearDisplay();
    display.setTextSize(3.5);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    switch(mState) {

        case STATE_STANDBY:
            display.println("Standby");
            break;

        case STATE_PRE_HEAT:
            display.setTextSize(2);
            display.println("Heizdauer");
            display.print(duration);
            display.println(" min");
            break;

        case STATE_HEAT:
            display.println("Heizen");
            display.print(duration);
            display.println(" min");
            break;

        case STATE_ERR:
            display.println("Fehler");
            break;

        default:
            break;
    }

    display.display();
}


/****************************************************************************/
/** Function
 */
void static ledRefresh(void) {
    switch(mState) {

        case STATE_HEAT:
            mStateLedGreen = !mStateLedGreen;
            digitalWrite(LED_G, mStateLedGreen);
            digitalWrite(LED_R, LOW);
            break;

        case STATE_ERR:
            mStateLedRed = !mStateLedRed;
            digitalWrite(LED_R, mStateLedRed);
            digitalWrite(LED_G, LOW);
            break;

        default:
            digitalWrite(LED_R, LOW);
            digitalWrite(LED_G, LOW);
            break;
    }
}


/****************************************************************************/
/** Function
 */
void static durationGet(uint16_t *pDuration) {
    *pDuration = analogRead(DURATION_PIN);
    *pDuration = map(*pDuration, 0, 1023, DURATION_MIN, DURATION_MAX);

    if (*pDuration < DURATION_MIN || *pDuration > DURATION_MAX) {
        *pDuration = DURATION_MAX;
    }
}


/****************************************************************************/
/** Function
 */
void static irqButton(void) {
    mFlgButtonPressed = true;
}


/****************************************************************************/
/** Function
 */
void setup(void) {
    ERR_STATUS result;

    result = peripheriInit();
    if (ERR_OK != result) {
        errorStateSet();
    }
}


/****************************************************************************/
/** Function
 */
void loop(void) {
    uint64_t timeCurr;
    uint64_t timeDisplay = 0;
    uint64_t timeLed = 0;
    uint64_t timeButton = 0;
    uint16_t duration = 30;

    while(1) {
        /* get timestamp */
        timeCurr = millis();

        /* state changing, with timeout to suppress debounce */
        if ((timeCurr > timeButton + BUTTON_LOOP_TIMEOUT) &&
            (true == mFlgButtonPressed)) {
            switch (mState) {
                case STATE_INIT:
                case STATE_STANDBY:
                case STATE_PRE_HEAT:
                    mState = mState + 1;
                    break;

                case STATE_HEAT:
                    mState = STATE_STANDBY;
                    break; 

                default:
                    break;
            }
            timeButton = millis();
            mFlgButtonPressed = false;
        }

        /* display loop */
        if (timeCurr > timeDisplay + DISPLAY_LOOP_TIMEOUT) {
            timeDisplay = millis();
            if (STATE_PRE_HEAT == mState) {
                durationGet(&duration);
            }
            displayRefresh(duration);
        }

        /* LED loop */
        if (timeCurr > timeLed + LED_LOOP_TIMEOUT) {
            timeLed = millis();
            ledRefresh();
        }

        /* TODO: implement loop for communication between
         * device and heater. Possibilities are 433 MHz RF,
         * Bluetooth and Smartphone or GSM? 
         */
    }
}
