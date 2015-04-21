#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// buttons are all on one analog pin
#define BUTTONS_PIN A0

// magic numbers are fun
#define RT_VAL 97
#define UP_VAL 254
#define DN_VAL 380
#define LT_VAL 638
#define SELECT_VAL 700

// photoresistor pin
#define PR_PIN A1

// start button pin
#define BUTTON 0

int upPressed = 0;
int dnPressed = 0;
int ltPressed = 0;
int rtPressed = 0;
int selectPressed = 0;

// limit start button presses to once every 250ms
unsigned long startPressLimitMicros = 250000;
unsigned long startTime = 0;

int dblClick = 1;

// up/down buttons can be used to manually adjust calibration by this much
unsigned int calibrationNudge = 10;
// auto calibration adds some percentage of error tolerance
double errTolerance = 0.33;
unsigned int calibration = 0;
unsigned int testDelay = 400;
unsigned int testRepeat = 100;
unsigned int randomizeAmt = 30;

unsigned long sum = 0;
unsigned long cnt = 0;
unsigned long prevResult = 0;
unsigned long maxResult = 0;
unsigned long minResult = 4294967295;

enum menu {
    START,
    CALIBRATE,
    DELAY,
    RANDOMIZE,
    DBLCLICK,
    REPEAT
};
enum menu currentMenu = START;

enum test {
    NOT_STARTED,
    STARTING,
    TESTING,
    ENDED
};
enum test testState = NOT_STARTED;

void setup() {
    pinMode(PR_PIN, INPUT_PULLUP);
    pinMode(BUTTONS_PIN, INPUT);
    pinMode(BUTTON, OUTPUT);
    digitalWrite(BUTTON, HIGH);
    randomSeed(analogRead(PR_PIN));
    lcd.begin(16, 2);
    lcdUpdate();

    // for debugging purposes only. don't leave serial prints in as they may cause extra latency
    //Serial.begin(9600);
}

void readButtons(int val) {
    if(val < RT_VAL) {
        upPressed = 0;
        dnPressed = 0;
        ltPressed = 0;
        rtPressed = rtPressed ? rtPressed : 1;
        selectPressed = 0;
    } else if (val < UP_VAL) {
        upPressed = upPressed ? upPressed : 1;
        dnPressed = 0;
        ltPressed = 0;
        rtPressed = 0;
        selectPressed = 0;
    } else if (val < DN_VAL) {
        upPressed = 0;
        dnPressed = dnPressed ? dnPressed : 1;
        ltPressed = 0;
        rtPressed = 0;
        selectPressed = 0;
    } else if (val < LT_VAL) {
        upPressed = 0;
        dnPressed = 0;
        ltPressed = ltPressed ? ltPressed : 1;
        rtPressed = 0;
        selectPressed = 0;
    } else if (val < SELECT_VAL) {
        upPressed = 0;
        dnPressed = 0;
        ltPressed = 0;
        rtPressed = 0;
        selectPressed = selectPressed ? selectPressed : 1;
    } else {
        upPressed = 0;
        dnPressed = 0;
        ltPressed = 0;
        rtPressed = 0;
        selectPressed = 0;
    }
}

unsigned long lastLcdUpdate = 0;
unsigned int refreshMillis = 500;

void lcdUpdate() {
    lastLcdUpdate = millis();
    lcd.clear();
    
    if (testState == NOT_STARTED) {
        switch (currentMenu) {
            case START:
                lcd.print("# Start #");
                break;
            case CALIBRATE:
                lcd.print("# Calibration #");
                lcd.setCursor(0, 1);
                lcd.print("trg:");
                lcd.print(calibration);
                lcd.print(" cur:");
                lcd.print(analogRead(PR_PIN));
                break;
            case DELAY:
                lcd.print("# Delay #");
                lcd.setCursor(0, 1);
                lcd.print(testDelay);
                lcd.print(" ms");
                break;
            case RANDOMIZE:
                lcd.print("# Delay jitter #");
                lcd.setCursor(0, 1);
                lcd.print("+/- ");
                lcd.print(randomizeAmt);
                lcd.print(" ms");
                break;
            case DBLCLICK:
                lcd.print("# Double click #");
                lcd.setCursor(0, 1);
                lcd.print(dblClick);
                break;
            case REPEAT:
                lcd.print("# Repeat #");
                lcd.setCursor(0, 1);
                lcd.print(testRepeat);
                lcd.print("x");
                break;
        }
    } else if (testState == ENDED) {
        lcd.print("Avg: ");
        lcd.print((double) sum / (double) cnt / 1000.0);
        lcd.print(" ms");
        lcd.setCursor(0, 1);
        if (minResult != 4294967295) {
            lcd.print((unsigned long int)(minResult / 1000.0));
            lcd.print("/");
        }
        if (maxResult) {
            lcd.print((unsigned long int)(maxResult / 1000.0));
        }
    } else if (testState == STARTING) {
        lcd.print("Sample: ");
        lcd.print(cnt);
        lcd.print("/");
        lcd.print(testRepeat);
        lcd.setCursor(0, 1);
        if (!prevResult) {
            lcd.print("Skipped sub-ms");
        } else if (prevResult == 4294967295) {
            lcd.print("Waiting...");
        } else {
            lcd.print("Prev:");
            lcd.print(floor(prevResult / 1000.0));
            lcd.print(" ms");
        }
    }

}

void loop() {
    int buttonVal = analogRead(BUTTONS_PIN);
    readButtons(buttonVal);
    
    // a button was just pressed
    if (upPressed == 1 || dnPressed == 1 || ltPressed == 1 || rtPressed == 1 || selectPressed == 1) {
        //Serial.println(buttonVal);
        if (testState == NOT_STARTED) {
            switch (currentMenu) {
                case START:
                    if (selectPressed) {
                        testState = STARTING;
                        sum = 0;
                        cnt = 0;
                        minResult = 4294967295;
                        maxResult = 0;
                        prevResult = 4294967295;
                    } else if (upPressed) {
                        currentMenu = REPEAT;
                    } else if (dnPressed) {
                        currentMenu = CALIBRATE;
                    }
                    break;
                case CALIBRATE:
                    if (selectPressed) {
                        // reset calibration
                        calibration = 1024;
                    } else if (upPressed) {
                        currentMenu = START;
                    } else if (dnPressed) {
                        currentMenu = DELAY;
                    } else if (ltPressed) {
                        calibration -= 20;
                    } else if (rtPressed) {
                        calibration += 20;
                    }
                    break;
                case DELAY:
                    if (upPressed) {
                        currentMenu = CALIBRATE;
                    } else if (dnPressed) {
                        currentMenu = RANDOMIZE;
                    } else if (ltPressed) {
                        testDelay -= 25;
                    } else if (rtPressed) {
                        testDelay += 25;
                    }
                    break;
                case RANDOMIZE:
                    if (upPressed) {
                        currentMenu = DELAY;
                    } else if (dnPressed) {
                        currentMenu = DBLCLICK;
                    } else if (ltPressed) {
                        randomizeAmt -= 5;
                    } else if (rtPressed) {
                        randomizeAmt += 5;
                    }
                    break;
                case DBLCLICK:
                    if (upPressed) {
                        currentMenu = RANDOMIZE;
                    } else if (dnPressed) {
                        currentMenu = REPEAT;
                    } else if (ltPressed || rtPressed || selectPressed) {
                        dblClick = !dblClick;
                    }
                    break;
                case REPEAT:
                    if (upPressed) {
                        currentMenu = DBLCLICK;
                    } else if (dnPressed) {
                        currentMenu = START;
                    } else if (ltPressed) {
                        testRepeat -= 5;
                    } else if (rtPressed) {
                        testRepeat += 5;
                    }
                    break;
            }
        } else if (testState == ENDED) {
            // dismiss test results
            testState = NOT_STARTED;
        } else {
            // button press ends active test
            testState = ENDED;
        }
        
        lcdUpdate();
        upPressed = upPressed ? 2 : 0;
        dnPressed = dnPressed ? 2 : 0;
        ltPressed = ltPressed ? 2 : 0;
        rtPressed = rtPressed ? 2 : 0;
        selectPressed = selectPressed ? 2 : 0;
    }
    
    // time before individual measurement when we update lcd / other preparations
    if (testState == STARTING) {
        lcdUpdate();
        
         unsigned int waitDelay = random(testDelay - randomizeAmt, testDelay + randomizeAmt);
        
        if (dblClick) {
            delay(waitDelay / 3.0);
            // quickly press button again to reset
            digitalWrite(BUTTON, LOW);
            delay(waitDelay / 3.0);
            // now depress button and wait some more
            digitalWrite(BUTTON, HIGH);
            delay(waitDelay / 3.0);
        } else {
            delay(waitDelay);
        }
        
        // start test by pressing button and storing time
        testState = TESTING;
        digitalWrite(BUTTON, LOW);
        startTime = micros();
    } else if (testState == TESTING) {
        if (analogRead(PR_PIN) < calibration) {
            prevResult = micros() - startTime;
            
            if (prevResult <= 1000) {
                // skip sub ms results
                prevResult = 0;
            } else {
                sum += prevResult;
                cnt++;
                
                if (prevResult < minResult) {
                    minResult = prevResult;
                }
                if (prevResult > maxResult) {
                    maxResult = prevResult;
                }
            }
            
            // depress button
            digitalWrite(BUTTON, HIGH);
            
            if (cnt >= testRepeat) {
                testState = ENDED;
                lcdUpdate();
            } else {
                testState = STARTING;
            }
        }
    } else if (testState == NOT_STARTED && currentMenu == CALIBRATE && selectPressed) {
        unsigned int val = analogRead(PR_PIN) * (1 - errTolerance);

        // update calibration if it's a new minimum
        if(val < calibration) {
            calibration = val;
            lcdUpdate();
        }
    }
    
    // depress button when not testing
    if (testState == NOT_STARTED || testState == ENDED) {
        digitalWrite(BUTTON, HIGH);
    }

    // update lcd periodically when in calibration screen
    if(millis() - lastLcdUpdate > refreshMillis && testState == NOT_STARTED && currentMenu == CALIBRATE) {
        lcdUpdate();
    }
}
