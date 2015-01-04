#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// buttons are all on one analog pin
#define BUTTONS_PIN A0

#define RT_VAL 98
#define UP_VAL 254
#define DN_VAL 407
#define LEFT_VAL 638
#define CALIBRATE_VAL 700

// photoresistor pin
#define PR_PIN A1

// start button pin
#define START_PIN 0

int upPressed = 0;
int dnPressed = 0;
int calibratePressed = 0;
int startPressed = 0;

// limit start button presses to once every 250ms
unsigned long startPressLimitMicros = 250000;
unsigned long startTime = 0;

int calibrationStarted = 0;
int testStarted = 0;

// up/down buttons can be used to manually adjust calibration by this much
unsigned int calibrationNudge = 10;
// auto calibration adds some percentage of error tolerance
double errTolerance = 0.1;
unsigned int calibration = 0;

unsigned long result = 0;

void setup() {
    pinMode(PR_PIN, INPUT_PULLUP);
    pinMode(BUTTONS_PIN, INPUT);
    pinMode(START_PIN, INPUT_PULLUP);
    lcd.begin(16, 2);

    // for debugging purposes only. don't leave serial prints in as they may cause extra latency
    //Serial.begin(9600);
}

void readButtons(int val) {
    //Serial.println(val);
    if(val < UP_VAL) {
        upPressed = 1;
        dnPressed = 0;
        calibratePressed = 0;
    } else if (val < DN_VAL) {
        upPressed = 0;
        dnPressed = 1;
        calibratePressed = 0;
    } else if (val < CALIBRATE_VAL) {
        upPressed = 0;
        dnPressed = 0;
        calibratePressed = 1;
    } else {
        upPressed = 0;
        dnPressed = 0;
        calibratePressed = 0;
    }
}

unsigned long lastLcdUpdate = 0;
unsigned int refreshMillis = 500;

void lcdUpdate() {
    if(upPressed)
        calibration += calibrationNudge;
    else if(dnPressed)
        calibration -= calibrationNudge;

    lastLcdUpdate = millis();
    lcd.clear();
    lcd.print("trg:");
    lcd.print(calibration);
    lcd.print(" cur:");
    lcd.print(analogRead(PR_PIN));
    lcd.setCursor(0, 1);
    if(testStarted) {
        lcd.print("testing...");
    } else {
        lcd.print("latency: ");
        lcd.print(result / 1000);
        lcd.print("ms");
    }
}

void loop() {
    readButtons(analogRead(BUTTONS_PIN));

    // test started and photoresistor value under calibration: show results
    if(testStarted && analogRead(PR_PIN) < calibration) {
        result = micros() - startTime;

        testStarted = 0;
        lcdUpdate();
    }

    // limit start button presses by time, avoids accidental presses
    if(!startPressed && !digitalRead(START_PIN) && (micros() - startTime > startPressLimitMicros || !startTime)) {
        // start button just pressed, "falling edge"
        startPressed = 1;
        testStarted = 1;

        startTime = micros();
    } else if (digitalRead(START_PIN)) {
        // start button depressed
        startPressed = 0;

        if(!calibrationStarted && calibratePressed) {
            // calibration button just pressed, "falling edge"
            calibrationStarted = 1;

            // reset calibration to highest value
            calibration = 1024;
            lcdUpdate();
        } else if (!calibratePressed) {
            // calibration button depressed
            calibrationStarted = 0;
        }

        if(calibrationStarted) {
            // calibration button still pressed, read current photoresistor value
            unsigned int val = analogRead(PR_PIN) * (1 - errTolerance);

            // update calibration if it's a new minimum
            if(val < calibration) {
                calibration = val;
                lcdUpdate();
            }
        }
    }

    // update lcd periodically
    // don't update if test started as writing to lcd takes time
    if(millis() - lastLcdUpdate > refreshMillis && !testStarted) {
        lcdUpdate();
    }
}
