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
const int sprayPort = -1;
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
char* topStringCur = "";
char* topStringPrev = "";
char* bottomStringPrev = "";
char* bottomStringCur = "";

// Debounce variables
int debounceDelay = 50;
unsigned long lastInterruptTime;
unsigned long lastAnalogButtonReadTime;

// Menu variables
int activeMenuItem = 2;
bool exitPressed = false;

// In Use variables
int usageMode = 0;
unsigned long inUseStartTime;
const unsigned long numberOneTime = 60000;
const unsigned long numberTwoTime = 6000;
int sprayAmount = 0;

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
  ">Set spray delay",
  ">Reset # shots",
  ">Exit"
};

void setup() {
  // put your setup code here, to run once:
  currentState = 1;
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
  // lcd.print("starting up");
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
      bottomStringCur = "Unexpected state";
      break;
  }

  printLCD();
}

void IdleChange() {
  if (motion == LOW){
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
  if (analogButtonCur == 0){
    currentState = 2;
    MenuActions();
  }
  // If the seat has been lifted.
  //else if (false){//magnetVoltPrev != magnetVoltCur){
  //  currentState = 3;
  //  InUseActions();
  //}
  // If the distance sensor no longer senses anything for 5 seconds.
  else if (false){//getDistance() <= 0){
    currentState = 0;
    if (usageMode == 3) {
      // If was cleaning, return to idle.
      currentState = 0;
      IdleActions();
    }
    else {
      // Else, spray.
      currentState = 3;
      SprayActions();
    }
    usageMode = 0;
  }
  else{
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
  if (sprayAmount == 0){
    currentState = 0;
    IdleActions();
  }
  else{
    SprayActions();
  }
}

void IdleActions(){
  // Do nothing.
  return;
}
void MenuActions(){
  // Show the menu options and currently selected one.
  bottomStringCur = menuItemNames[activeMenuItem];

  if (analogButtonPrev == -1) {
    if (analogButtonCur == 0) {
      // If the first button has been pressed:
      activeMenuItem++;
      activeMenuItem %= 3;
    }
  
    if (analogButtonCur == 1) {
      if (activeMenuItem == 2) {
        // If the second button has been pressed:
        exitPressed = true;
        activeMenuItem = 2;
      }
    }
  }
}
void InUseActions(){
  switch (usageMode) {
    case 0: // Unknown
      // Measure for 15 seconds how much movement there is.
      // If there is a lot of movement, switch to 3/cleaning.
      // Else, switch to 1.
      usageMode = 1;
      break;
    case 1: // Number 1
      // After one minute, switch to 2.
      if (millis() - inUseStartTime >= numberTwoTime) {
        usageMode = 2;
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
  return;
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

int getDistance() {
  // Get the distance from the distance sensor.
  return sonar.ping_cm();
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
    currentState = 3;
  }
  buttonPrev = buttonCur;
}

