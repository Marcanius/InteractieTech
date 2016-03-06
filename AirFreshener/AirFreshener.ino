// Libraries
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NewPing.h>

// A byte representing the state we are in.
volatile int currentState;

// Ports
const byte motionPort = 3, tempPort = 9;
const byte echoPort = 8, triggerPort = 10;
const byte buttonPort = 2;
const byte analogButtonsPort = A1;
const byte sprayPort = A2;
const byte lightPort = A0;
LiquidCrystal lcd(12, 11, 5, 4, 6, 7);
OneWire oneWire(tempPort);
DallasTemperature tempSensor(&oneWire);
NewPing sonar(triggerPort, echoPort, 100);

// SensorData
//int magnetVoltCur, magnetVoltPrev;
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
const unsigned long numberOneTime = 6000;
const unsigned long numberTwoTime = 5000;
unsigned long lastCheckTimee;
int timesNoOneThere;

// Spray variables
volatile int sprayAmount = 0;
unsigned long sprayStartTime;
byte sprayDelays[] = { 1, 2, 5, 10, 15, 20, 30, 45, 60 };
byte sprayDelay = 2;
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
  ">Spray delay: ",
  ">Reset # shots",
  ">Exit"
};

void setup() {
  // put your setup code here, to run once:
  //currentState = 1;
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
  //Serial.begin(9600);
  
  delay(1000);
  interrupts();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Update sensor vals.
  // Write away the previous values.
  analogButtonPrev = analogButtonCur;
  
  // Sense the new values.
  //motion = digitalRead(motionPort);
  analogButtonCur = getAnalogButtonPressed();
  
  topStringCur = stateNames[currentState];
  
  // Change States
  switch(currentState) {
    case 0: // Idle
      IdleChange();
      break;
    case 1: // InUse
      InUseChange();
      break;
    case 2: // Menu
      MenuChange();
      break;
    case 3: // Spray
      SprayChange();
      break;
    default:
      topStringCur = currentState + 48;
      break;
  }

  //digitalWrite(13, digitalRead(motionPort));
  if (noOneThere()) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }
  printLCD();
}

void IdleChange() {
  if (!noOneThere()){
    //lcd.print("a");
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
  // If a menu button is pressed.
  if (analogButtonCur == 0) {
    currentState = 2;
    MenuActions();
  }
  // If the distance sensor no longer senses anything for 5 seconds.
  else if (noOneThere()) {
    usageMode = 0;
    if (usageMode == 3) {
      // If was cleaning, return to idle.
      currentState = 0;
      spraying = false;
      IdleActions();
    }
    if (usageMode < 3) {
      // Else, spray.
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
  // If the quit option has been taken.
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
  // When we have finished spraying.
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
    bottomStringCur += String(sprayDelays[sprayDelay]);
  }

  if (analogButtonCur == -1) {
    switch (analogButtonPrev) {
      case 0:
        activeMenuItem++;
        activeMenuItem %= 3;
        break;
      case 1:
        switch (activeMenuItem) {
          case 0:
            sprayDelay++;
            sprayDelay %= 9;
            break;
          case 1:
            spraysLeft = maxSpraysLeft;
            break;
          case 2:
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
  
  if (currentTime - tempUpdatedTime >= 2000) {
    tempString = "      " + String(getTemperature()) + (char)223 + "C";
    tempUpdatedTime = currentTime;
  }
  topStringCur += tempString;
  
  switch (usageMode) {
    case 0: // Unknown
      // Measure for 15 seconds how much movement there is.
      // If there is a lot of movement, switch to 3/cleaning.
      // Else, switch to 1.
      sprayAmount = 0;
      if (currentTime - inUseStartTime <= numberOneTime) {
        timesChecked++;
        if (motion == HIGH) {
          highTimes++;
        }
      }
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
      // After some time, switch to 2.
      if (currentTime - inUseStartTime >= numberTwoTime + numberOneTime) {
        usageMode = 2;
        //spraying = true;
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
  bottomStringCur = usageNames[usageMode];
}
void SprayActions(){
  // Spray.
  if (sprayAmount == 0) {
    spraying = false;
    return;
  }

  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - sprayStartTime;
  if (timePassed >= (unsigned long)sprayDelays[sprayDelay] * 1000 + 1500) {
    sprayStartTime = currentTime;
    sprayAmount--;
    spraysLeft--;
    if (sprayAmount <= 0) {
      spraying = false;
    }
  }
  else if (timePassed >= (unsigned long)(sprayDelays[sprayDelay] + 1) * 1000) {
    digitalWrite(sprayPort, LOW);
  }
  else {
    topStringCur += " in " + String(sprayDelays[sprayDelay] - timePassed / 1000);
    bottomStringCur = String(spraysLeft) + F(" shots left");
    digitalWrite(sprayPort, HIGH);
  }
}

bool noOneThere() {
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTimee >= 500) {
    lastCheckTimee = currentTime;
    if (sonar.ping_cm() == 0 && motion == LOW) {
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
  if (value <= 10) {
    // The first button is pressed.
    return 0;
  }
  else if (value >= 502 && value <= 522) {
    // The second button is pressed.
    return 1;
  }
  else {
    // No button is pressed (assuming only two buttons are connected).
    return -1;
  }
}

int getTemperature() {
  // Get the temperature from the temperature sensor.
  tempSensor.requestTemperatures();
  return tempSensor.getTempCByIndex(0);
}

bool isBouncing(unsigned long lastTime) {
  unsigned long currentTime = millis();
  return currentTime - lastTime <= debounceDelay;
}

void printLCD() {
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

  buttonCur = digitalRead(buttonPort);
  if (buttonPrev == LOW && buttonCur == HIGH) {
    lastInterruptTime = millis();
    sprayAmount = 1;
    spraying = true;
    sprayStartTime = millis();
    currentState = 3;
  }
  buttonPrev = buttonCur;
}

void motionInterrupt() {
  // Interrupt function for the motion sensor.

  motion = digitalRead(motionPort);
}

