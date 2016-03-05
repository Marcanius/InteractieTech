// Libraries
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NewPing.h>

// A byte representing the state we are in.
volatile byte currentState;

// Ports
const int motionPort = -1, magnetPort = -1, lightPort = -1, tempPort = -1;
const int echoPort = -1, triggerPort = -1;
const int buttonPort = 2;
const int analogButtonsPort = A0;
const int sprayPort = 13;
LiquidCrystal lcd(12, 11, 7, 6, 5, 4);
OneWire oneWire(tempPort);
DallasTemperature tempSensor(&oneWire);
NewPing sonar(triggerPort, echoPort, 300);

// SensorData
int magnetVoltCur, magnetVoltPrev;
int motion;
int buttonPrev, buttonCur;
int analogButtonPrev, analogButtonCur;
int tempCur, tempPrev;

// LCD variables
String topStringCur;
String topStringPrev;
String bottomStringPrev;
String bottomStringCur;
String tempString;

// Debounce variables
int debounceDelay = 50;
unsigned long lastInterruptTime;
unsigned long lastAnalogButtonReadTime;

// Menu variables
int activeMenuItem = 2;
bool exitPressed = false;

// In Use variables
int usageMode;
unsigned long inUseStartTime;
unsigned long tempUpdatedTime;
//const unsigned long numberOneTime = 60000;
const unsigned long numberTwoTime = 6000;

// Spray variables
volatile int sprayAmount = -1;
unsigned long sprayStartTime;
int sprayDelays[] = { 1, 2, 5, 10, 15, 20, 30, 45, 60 };
int sprayDelay = 2;
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
  pinMode(magnetPort, INPUT);
  pinMode(lightPort, INPUT);
  pinMode(buttonPort, INPUT);
  pinMode(tempPort, INPUT);

  // Set the interrupts
  attachInterrupt(digitalPinToInterrupt(buttonPort), sprayInterrupt, CHANGE);

  // Start temperature sensor
  tempSensor.begin();

  buttonPrev = HIGH;
  
  lcd.begin(16,2);
  
  delay(1000);
  interrupts();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Update sensor vals.
  // Write away the previous values.
  magnetVoltPrev = magnetVoltCur;
  analogButtonPrev = analogButtonCur;
  
  // Sense the new values.
  magnetVoltCur = digitalRead(magnetPort);
  motion = digitalRead(motionPort);
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
      topStringCur = "ERROR:";
      bottomStringCur = "Unexp. state: " + String(currentState);
      break;
  }

  printLCD();
}

void IdleChange() {
  if (true){//motion == LOW){
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
  // If the seat has been lifted.
  //else if (false){//magnetVoltPrev != magnetVoltCur) {
  //  currentState = 3;
  //  InUseActions();
  //}
  // If the distance sensor no longer senses anything for 5 seconds.
  else if (spraying){//getDistance() <= 0) {
    if (usageMode == 3) {
      // If was cleaning, return to idle.
      currentState = 0;
      spraying = false;
      IdleActions();
    }
    else {
      // Else, spray.
      currentState = 3;
      sprayStartTime = millis();
      SprayActions();
    }
    usageMode = 0;
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
    tempString = "     " + String(getTemperature()) + (char)223 + "C";;
    tempUpdatedTime = currentTime;
  }
  topStringCur += tempString;
  
  switch (usageMode) {
    case 0: // Unknown
      // Measure for 15 seconds how much movement there is.
      // If there is a lot of movement, switch to 3/cleaning.
      // Else, switch to 1.
      usageMode = 1;
      break;
    case 1: // Number 1
      // After one minute, switch to 2.
      if (currentTime - inUseStartTime >= numberTwoTime) {
        //usageMode = 2;
        //currentState = 3;
        spraying = true;
      }
      sprayAmount = 1;
      break;
    case 2: // Number 2
      sprayAmount = 2;
      break;
    case 3: // Cleaning
      break;
    default:
      break;
  }
  bottomStringCur = usageNames[usageMode];
}
void SprayActions(){
  // Spray.
  if (!spraying) {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - sprayStartTime;
  if (timePassed >= sprayDelays[sprayDelay] * 1000 + 1500) {
    sprayStartTime = currentTime;
    sprayAmount--;
    spraysLeft--;
    if (sprayAmount == 0) {
      spraying = false;
    }
  }
  else if (timePassed >= (sprayDelays[sprayDelay] + 1) * 1000) {
    digitalWrite(sprayPort, LOW);
  }
  else {
    topStringCur += " in " + String(sprayDelays[sprayDelay] - timePassed / 1000);
    bottomStringCur = String(spraysLeft) + " shots left";
    digitalWrite(sprayPort, HIGH);
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

