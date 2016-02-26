// A byte representing the state we are in
byte currentState;
// Ports

// SensorData

void setup() {
  // put your setup code here, to run once:
  currentState = 1;
  // Set the ports
}

void loop() {
  // put your main code here, to run repeatedly:
  // Update sensor vals.
    // Write away the previous values.
    // Sense the new values.
  
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
  if (false){
    currentState = 1;
    SoTActions();
  }
  else {
    IdleActions();
  }
}
void SoTChange(){
  if (false){
    currentState = 2;
    MenuActions();
  }
  else if (false){
    currentState = 3;
    InUseActions();
  }
  else if (false){
    currentState = 0;
    IdleActions();
  }
  else{
    SoTActions();
  }
}
void MenuChange(){
  if (false){
    currentState = 1;
    SoTActions();
  }
  else{
    MenuActions();
  }
}
void InUseChange(){
  if (false){
    currentState = 4;
    SprayActions;
  }
  else{
    InUseActions();
  }
}
void SprayChange(){
  if (false){
    currentState = 1;
    SoTActions();
  }
  else{
    SprayActions;
  }
}

void IdleActions(){
  return;
}
void SoTActions(){
  return;
}
void MenuActions(){
  return;
}
void InUseActions(){
  return;
}
void SprayActions(){
  return;
}

