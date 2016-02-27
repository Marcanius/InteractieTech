// Libraries
#include <LiquidCrystal.h>

// A byte representing the state we are in.
byte currentState;

// Ports
const int motionPort = -1, distancePort = -1 , magnetPort = -1, lightPort = -1, tempPort = -1;
const int buttonOnePort = 7, buttonTwoPort = -1, buttonThreePort = -1;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// SensorData
int magnetVoltCur, magnetVoltPrev;
int motion;
int buttonOnePrev, buttonOneCur;
int buttonTwoPrev, buttonTwoCur; 
int buttonThreePrev, buttonThreeCur;
int tempCur, tempPrev;

void setup() {
  // put your setup code here, to run once:
  currentState = 1;
  // Set the ports
  pinMode(motionPort, INPUT);
  pinMode(distancePort, INPUT);
  pinMode(magnetPort, INPUT);
  pinMode(lightPort, INPUT);
  pinMode(buttonOnePort, INPUT);
  pinMode(buttonTwoPort, INPUT);
  pinMode(buttonThreePort, INPUT);
  pinMode(tempPort, INPUT);
  
  lcd.begin(16,2);
  // lcd.print("starting up");
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Update sensor vals.
  // Write away the previous values.
  magnetVoltPrev = magnetVoltCur;
  buttonOnePrev = buttonOneCur;
  buttonTwoPrev = buttonOneCur;
  buttonThreePrev = buttonThreeCur;
  
  // Sense the new values.
  magnetVoltCur = digitalRead(magnetPort);
  motion = digitalRead(motionPort);
  buttonOneCur = digitalRead(buttonOnePort);
  buttonTwoCur = digitalRead(buttonTwoPort);
  buttonThreeCur = digitalRead(buttonThreePort);
  
  // print the current state
  lcd.setCursor(0,0);
  lcd.print("state = ");
  lcd.print(currentState);
  lcd.setCursor(0,1);
  lcd.print(digitalRead(buttonOnePort));
  
  // Change States
  switch(currentState) {
    case 0: // Idle
      IdleChange();
      break;
    case 1: // SomeoneThere
      SoTChange();
      break;
    case 2: // Menu
      MenuChange();
      break;
    case 3: // In Use
      InUseChange();
      break;
    case 4: // Spray
      SprayChange();
      break;
    /*
    case 5:
      break;
    case 6:
      break;
    case 7:
      break;
      */
    default:
      break;
  }
}

void IdleChange() {
  if (motion == LOW){
    currentState = 1;
    SoTActions();
  }
  else {
    IdleActions();
  }
}
void SoTChange(){
  // If the menu button is pressed.
  if (buttonOneCur == LOW){
    currentState = 2;
    MenuActions();
  }
  // If the seat has been lifted.
  else if (magnetVoltPrev != magnetVoltCur){
    currentState = 3;
    InUseActions();
  }
  // If the distance sensor no longer senses anything.
  else if (false){
    currentState = 0;
    IdleActions();
  }
  else{
    SoTActions();
  }
}
void MenuChange(){
  // If the quit option has been taken.
  if (false){
    currentState = 1;
    SoTActions();
  }
  else{
    MenuActions();
  }
}
void InUseChange(){
  // If we have finished our business.
  if (false){
    currentState = 4;
    SprayActions;
  }
  else{
    InUseActions();
  }
}
void SprayChange(){
  // When we have finished spraying.
  if (false){
    currentState = 1;
    SoTActions();
  }
  else{
    SprayActions;
  }
}

void IdleActions(){
  // Do nothing.
  return;
}
void SoTActions(){
  // Show that we are in the SoT state (via LED or LCD).
  return;
}
void MenuActions(){
  // Show the menu options and currently selected one.
  return;
}
void InUseActions(){
  // Show that we are in the In Use state (via LED or LCD).
  return;
}
void SprayActions(){
  // Spray.
  return;
}

