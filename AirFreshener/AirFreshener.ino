#include <NewPing.h>

// Libraries
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NewPing.h>

// A byte representing the state we are in.
volatile int currentState;

// Ports
const byte motionPort = 3, tempPort = 9;
const byte echoPort = 10, triggerPort = 8;
const byte buttonPort = 2;
const byte analogButtonsPort = A1;
const byte sprayPort = A2;
const byte lightPort = A0;
const byte ledPort1 = 13, ledPort2 = A3;
LiquidCrystal lcd(12, 11, 5, 4, 6, 7);
OneWire oneWire(tempPort);
DallasTemperature tempSensor(&oneWire);
NewPing sonar(triggerPort, echoPort, 100);

// SensorData
volatile byte motion;
byte buttonPrev, buttonCur;
char analogButtonPrev, analogButtonCur;

// LCD variables
String topStringCur;
String topStringPrev;
String bottomStringPrev;
String bottomStringCur;
String tempString;

// Debounce variables
const byte debounceDelay = 50;
unsigned long lastInterruptTime;
unsigned long lastAnalogButtonReadTime;

// Menu variables
byte activeMenuItem = 2;
bool exitPressed = false;

// In Use variables
byte usageMode;
unsigned long inUseStartTime;
unsigned long tempUpdatedTime;
// In whole percentages, how much % of the time the motion sensor
// has to give HIGH to switch to 'cleaning' instead of 'number 1'.
const int cleaningMotionPercentage = 95;
unsigned long timesChecked, highTimes;
const unsigned long numberOneTime = 15000;
const unsigned long numberTwoTime = 60000;
unsigned long lastCheckTimee;
int timesNoOneThere;

// Spray variables
volatile int sprayAmount = 0;
unsigned long sprayStartTime;
unsigned int sprayDelays[] = { 15, 30, 45, 60, 90, 120, 180, 240, 300 };
byte sprayDelay = 0;
const unsigned int airFreshenerDelay = 15;
const int maxSpraysLeft = 2400;
int spraysLeft = maxSpraysLeft;
volatile bool spraying = false;

char* stateNames[] = {
  "Not in use", // Idle
  "In use", // InUse
  "Menu",
  "Spraying"
};
char* usageNames[] = {
  "...", // SomeoneThere
  "Number 1",
  "Number 2",
  "Cleaning"
};
char* menuItemNames[] = {
  ">Spray delay:",
  ">Reset # shots",
  ">Exit"
};

void setup() {
  noInterrupts();
  // Set the ports
  pinMode(motionPort, INPUT);
  pinMode(buttonPort, INPUT);
  pinMode(tempPort, INPUT);

  // Set the interrupts
  attachInterrupt(digitalPinToInterrupt(buttonPort), sprayInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(motionPort), motionInterrupt, CHANGE);

  // Start temperature sensor
  tempSensor.begin();

  buttonPrev = HIGH;
  
  lcd.begin(16,2);
  
  delay(1000);
  interrupts();
}

void loop() {
  // Get which button has been pressed.
  analogButtonPrev = analogButtonCur;
  analogButtonCur = getAnalogButtonPressed();

  // Set the text on the first line of the LCD
  // to show which state we are in.
  topStringCur = stateNames[currentState];
  
  // Change States
  switch(currentState) {
    case 0: // Idle
      IdleChange();
      break;
    case 1: // In Use
      InUseChange();
      break;
    case 2: // Menu
      MenuChange();
      break;
    case 3: // Spray
      SprayChange();
      break;
    default: // Unknown state
      topStringCur = currentState + 48; // Error message
      break;
  }

  if (currentState != 0) {
    analogWrite(ledPort2, 1023);
  }
  else {
    analogWrite(ledPort2, 0);
  }
  
  printLCD();
}

void IdleChange() {
  // If someone's there, go to In Use.
  if (!noOneThere()) {
    currentState = 1;
    usageMode = 0;
    inUseStartTime = millis();
    InUseActions();
  }
  else {
    IdleActions();
  }
}
void InUseChange(){
  // If the (first) menu button is pressed, go to Menu.
  if (analogButtonPrev == 0 && analogButtonCur == -1) {
    currentState = 2;
    MenuActions();
  }
  // If no one's there (for 5 seconds), go to Spraying or Idle
  else if (noOneThere()) {
    usageMode = 0;
    // If usage mode was Cleaning or Unknown, go to to idle.
    if (usageMode == 3) {
      currentState = 0;
      spraying = false;
      IdleActions();
    }
    else {
      // Else (if usage mode was Number 1 or 2), go to Spraying.
      currentState = 3;
      spraying = true;
      sprayStartTime = millis();
      SprayActions();
    }
  }
  else {
    InUseActions();
  }
}
void MenuChange(){
  // If the Exit option has been pressed, go to Idle.
  if (exitPressed){
    currentState = 0;
    exitPressed = false;
    IdleActions();
  }
  else{
    MenuActions();
  }
}
void SprayChange(){
  // If we have finished spraying, go to Idle.
  if (!spraying){
    currentState = 0;
    IdleActions();
  }
  else{
    SprayActions();
  }
}

void IdleActions(){
  // Do nothing.
  bottomStringCur = "";
  return;
}
void MenuActions(){
  // Show the menu options and currently selected one.
  bottomStringCur = menuItemNames[activeMenuItem];
  if (activeMenuItem == 0) {
    if (sprayDelays[sprayDelay] < 100) {
      bottomStringCur += " ";
    }
    bottomStringCur += String(sprayDelays[sprayDelay]);
  }

  // Handle pressing the menu items.
  // If no button is currently pressed:
  if (analogButtonCur == -1) {
    switch (analogButtonPrev) {
      case 0:
        // If the menu/switch (first) button was pressed,
        // go to the next menu item.
        activeMenuItem++;
        activeMenuItem %= 3;
        break;
      case 1:
        // If the select (second) button was pressed:
        switch (activeMenuItem) {
          case 0:
            // If the first menu item was selected,
            // switch between the spray delays.
            sprayDelay++;
            sprayDelay %= 9;
            break;
          case 1:
            // If the second menu item was selected,
            // reset the amount of spray shots left.
            spraysLeft = maxSpraysLeft;
            break;
          case 2:
            // If the third menu item was selected,
            // quit the menu.
            exitPressed = true;
            activeMenuItem = 2;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}
void InUseActions(){
  unsigned long currentTime = millis();

  // Show the temperature.
  if (currentTime - tempUpdatedTime >= 2000) {
    tempString = "      " + String(getTemperature()) + (char)223 + "C";
    tempUpdatedTime = currentTime;
  }
  topStringCur += tempString;

  // Do different things depending on the current usage mode.
  switch (usageMode) {
    case 0: // Unknown
      // Measure for x seconds how much movement there is.
      // If there is a lot of movement, switch to 3/cleaning.
      // Else, switch to 1.
      sprayAmount = 0;
      
      // During the x seconds, check how much movement there is.
      if (currentTime - inUseStartTime <= numberOneTime) {
        timesChecked++;
        if (motion == HIGH) {
          highTimes++;
        }
      }
      // After the x seconds, go to Cleaning or Number 1
      // depending on how much movement there is.
      else {
        if (highTimes * 100 / timesChecked >= cleaningMotionPercentage) {
          usageMode = 3;
        }
        else {
          usageMode = 1;
        }
        
        timesChecked = 0;
        highTimes = 0;
      }
      break;
    case 1: // Number 1
      // After y seconds, switch to 2.
      if (currentTime - inUseStartTime >= numberTwoTime + numberOneTime) {
        usageMode = 2;
      }
      sprayAmount = 1;
      break;
    case 2: // Number 2
      sprayAmount = 2;
      break;
    case 3: // Cleaning
      sprayAmount = 0;
      break;
    default:
      break;
  }

  // Show the usage mode on the bottom line of the LCD screen
  bottomStringCur = usageNames[usageMode];

  int bottomStringLength = bottomStringCur.length();
  String inUseTime = String((currentTime - inUseStartTime) / 1000);
  int inUseTimeLength = inUseTime.length();
  for (int i = 0; i < 16 - bottomStringLength - inUseTimeLength - 1; i++) {
    bottomStringCur += " ";
  }
  bottomStringCur += inUseTime + "s";
}
void SprayActions(){
  if (sprayAmount == 0) {
    spraying = false;
    return;
  }

  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - sprayStartTime;

  // During the sprayDelay - airFreshenerDelay, do nothing.
  if (timePassed < (unsigned long)(sprayDelays[sprayDelay] - airFreshenerDelay) * 1000) {

  }
  // After sprayDelay - airFreshenerDelay seconds until sprayDelay + 1 seconds, turn on the mosfet/air freshener.
  else if (timePassed < (unsigned long)sprayDelays[sprayDelay] * 1000 + 1000) {
    analogWrite(sprayPort, 1023);
  }
  // After sprayDelay + 1 seconds until sprayDelay + 1.5 seconds, turn off the mosfet/air freshener.
  else if (timePassed < (unsigned long)sprayDelays[sprayDelay] * 1000 + 1500) {
    analogWrite(sprayPort, 0);
  }
  // After sprayDelay + 1.5 seconds, stop spraying or spray again.
  else {
    sprayStartTime = currentTime;
    sprayAmount--;
    spraysLeft--;
    if (sprayAmount <= 0) {
      spraying = false;
    }
  }

  // Let the (first) LED blink.
  digitalWrite(ledPort1, (timePassed / 500 + 1) % 2);

  // Show how many seconds are left until the air freshener starts spraying.
  topStringCur += " in " + String(sprayDelays[sprayDelay] - min(timePassed / 1000, sprayDelays[sprayDelay]));
  
  // Switch between what to show on (the bottom line of) the LCD screen.
  unsigned long modulo = timePassed % 10000;
  if (modulo < 4000) {
    bottomStringCur = String(spraysLeft) + " shots left";
  }
  else if (modulo < 7000) {
    bottomStringCur = "Don't forget to";
  }
  else {
    bottomStringCur = "close the lid!";
  }
}

bool noOneThere() {
  // Check every 0.5 seconds if:
  // * the distance sensor does not see someone;
  // * the motion sensor senses no motion;
  // * and the toilet lid is closed.
  // If all of these are true for 5 seconds, then no one is there.
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTimee >= 500) {
    lastCheckTimee = currentTime;
    if (sonar.ping_cm() == 0 && motion == LOW && getAnalogButtonPressed() == 2) {
      timesNoOneThere++;
    }
    else {
      timesNoOneThere = 0;
    }
  }
  if (timesNoOneThere >= 2 * 5) {
    return true;
  }
  else {
    return false;
  }
}

int getAnalogButtonPressed() {
  // Check which button is pressed.

  // Debounce.
  if (isBouncing(lastAnalogButtonReadTime)) {
    return analogButtonPrev;
  }
  lastAnalogButtonReadTime = millis();
  
  int value = analogRead(analogButtonsPort);
  if (value > 1000) {
    // No button is pressed.
    return -1;
  }
  else if (value > 670) {
    // The magnetic sensor returns high.
    return 2;
  }
  else if (value > 500) {
    // The second button is pressed.
    return 1;
  }
  else {
    // The first button is pressed.
    return 0;
  }
}

int getTemperature() {
  // Get the temperature from the temperature sensor.
  tempSensor.requestTemperatures();
  return tempSensor.getTempCByIndex(0);
}

bool isBouncing(unsigned long lastTime) {
  // Debounce by ignoring all changes after
  // the first change for debounceDelay milliseconds.
  unsigned long currentTime = millis();
  return currentTime - lastTime <= debounceDelay;
}

void printLCD() {
  // If the text the LCD has to show has been changed,
  // then clear the LCD screen and show the new text.
  // This is done to prevent the LCD screen from clearing
  // and printing extremely fast.
  if (topStringPrev != topStringCur || bottomStringPrev != bottomStringCur) {
    lcd.clear();
    lcd.print(topStringCur);
    lcd.setCursor(0,1);
    lcd.print(bottomStringCur);
    topStringPrev = topStringCur;
    bottomStringPrev = bottomStringCur;
  }
}

void sprayInterrupt() {
  // Interrupt function for the spray button.

  // Debounce.
  if (isBouncing(lastInterruptTime)) {
    return;
  }

  // If the spray button has been pressed (and released),
  // go to Spraying and spray one more time.
  buttonCur = digitalRead(buttonPort);
  if (buttonPrev == HIGH && buttonCur == LOW) {
    lastInterruptTime = millis();
    if (!spraying) {
      sprayAmount = 1;
      spraying = true;
      sprayStartTime = millis();
      currentState = 3;
    }
    else {
      sprayAmount++;
    }
  }
  buttonPrev = buttonCur;
}

void motionInterrupt() {
  // Interrupt function for the motion sensor.

  motion = digitalRead(motionPort);
}

