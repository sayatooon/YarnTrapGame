#include <Arduino.h>

#include <DFRobotDFPlayerMini.h>
#include <LiquidCrystal_I2C.h>

// state
#define WAITING 0
#define SENSING 1
#define VIBRATION 2
#define BUTTON 3
#define FINISH 4
// button color
#define WHITE 0
#define BLUE 1
#define YELLOW 2
#define RED 3
#define GREEN 4
// sound
#define START_SOUND 1
#define BUTTON_TRUE_SOUND 2
#define BUTTON_FALSE_SOUND 3
#define VIBRATIONS_SOUND 4
#define GAMECLEAR1_SOUND 5
#define GAMECLEAR2_SOUND 6
#define GAMEOVER1_SOUND 7
#define GAMEOVER2_SOUND 8

// pin setting
const int PIN_BTN[] = {25, 26, 27, 14, 32}; // button digital I/O
const int PIN_VIB = 33; // vibration sensor digital I/O
const int PIN_PTM = 4; // potentiometer analog I/O
const int PIN_DFP = 23; // DFplayer busy 
//const int PIN_TX1 = 18, PIN_RX1 = 19; // DF player UART

// device setting

HardwareSerial HardwareSerial(2); // UART2 RX=GPIO16, TX=GPIO17 for the DFplayer
//SoftwareSerial mySoftwareSerial(PIN_RX1, PIN_TX1); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// vairables for the game
int numVib = 0; // the number of vibrations
int numBtnF = 0; // the number of the wrong button presses
int numBtnT = 0; // the number of the right button presses
int order_value; // the number of the order for free mode 
int order[8][3] = {{1,2,3}, {1,3,2}, {2,1,3}, {2,3,1}, {3,1,2}, {3,2,1}, {0,0,0}, {0,0,0}}; // button push order(1:bule, 2:yellow, 3:red)
String orderString[6] = {"BLUE>YELLOW>RED", "BLUE>RED>YELLOW", "YELLOW>BLUE>RED", "YELLOW>RED>BLUE", "RED>BULE>YELLOW", "RED>YELLOW>BLUE"};
int btnCursol[4] = {0,0,0,15}; // the cursol position to show a current button to be pressed

int state = WAITING; // game state
int prevMode = 10, currMode =0; // game mode
String mode[] ={"MODE1 ", "MODE2 ", "MODE3 ", "MODE4 ", "MODE5 ", "MODE6 ", "RANDOM", "SECRET"}; // 6 charactars
// vairables for buttons and sensor
bool push[] = {false, false, false, false, false}, sens = false; // flag of button/sensor interrupt 
bool buttonState[] = {HIGH, HIGH, HIGH, HIGH, HIGH}, lastButtonState[] = {HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[5];
unsigned long debounceWindow = 10; // [ms] ajust depending on the buttons
int novib=0;

// function prototype
void push_btn_W();
void push_btn_B();
void push_btn_Y();
void push_btn_R();
void push_btn_G();
void sens_vib();
void game_over();
void game_clear();
int isPushbuttonClicked(int i);


void setup() {
  Serial.begin(115200);
  HardwareSerial.begin(9600);
  //mySoftwareSerial.begin(9600);

  pinMode( PIN_BTN[WHITE], INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(PIN_BTN[WHITE]),push_btn_W,FALLING);
  pinMode( PIN_BTN[BLUE], INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(PIN_BTN[BLUE]),push_btn_B,FALLING);
  pinMode( PIN_BTN[YELLOW], INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(PIN_BTN[YELLOW]),push_btn_Y,FALLING);
  pinMode( PIN_BTN[RED], INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(PIN_BTN[RED]),push_btn_R,FALLING);
  pinMode( PIN_BTN[GREEN], INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(PIN_BTN[GREEN]),push_btn_G,FALLING);
  pinMode( PIN_VIB, INPUT_PULLUP );
  attachInterrupt(digitalPinToInterrupt(PIN_VIB),sens_vib,FALLING);
  pinMode( PIN_DFP, INPUT );

  // LCD setting
  lcd.init();
  lcd.backlight();
  //lcd.setCursor(0, 0);
  //lcd.print("LCD connect.    ");

  //speaker setting
  if (!myDFPlayer.begin(HardwareSerial)) {  //Use softwareSerial to communicate with mp3.
    lcd.setCursor(0, 0);
    lcd.print("Speaker fail.   ");
    lcd.setCursor(0, 1);
    lcd.print("Try restrat.");
    while(1);
  }else{
    myDFPlayer.volume(30); //Set volume value. From 0 to 30  
  }
  lcd.setCursor(0, 0);
  lcd.print("    WELCOME!");
  lcd.setCursor(0, 1);
  lcd.print("YARN TRAP GAME");
  delay(1000);

}

void loop() {

  switch (state) {
  case WAITING:
    // check the mode
    currMode = map(analogRead(PIN_PTM), 0, 4095, 0, 7);
    if (currMode != prevMode){
      Serial.println(mode[currMode]);
      prevMode = currMode;
      lcd.clear();
    
      if (currMode == 6 || currMode == 7){
        order_value = random(0,5);
        order[currMode][0] = order[order_value][0];
        order[currMode][1] = order[order_value][1];
        order[currMode][2] = order[order_value][2];
        lcd.setCursor(0, 0);
        lcd.print(mode[currMode] + "  T0 F0 A0");
        lcd.setCursor(0, 1);
        if (currMode == 6){
          lcd.print(orderString[order_value]);
          btnCursol[1] = orderString[order_value].indexOf(">")+1;
          btnCursol[2] = orderString[order_value].lastIndexOf(">")+1;
        }else{
          lcd.print("                ");
        }        
      }else{
        lcd.setCursor(0, 0);
        lcd.print(mode[currMode] + "  T0 F0 A0");
        lcd.setCursor(0, 1);
        lcd.print(orderString[currMode]);
        btnCursol[1] = orderString[currMode].indexOf(">")+1;
        btnCursol[2] = orderString[currMode].lastIndexOf(">")+1;
      }
    }
    
    if (push[WHITE]){  
      if (isPushbuttonClicked(WHITE) == 1){
        numVib = 0; 
        numBtnF = 0;
        numBtnT = 0;
        lcd.setCursor(btnCursol[numBtnT], 1);
        lcd.blink();
        state = SENSING;
        myDFPlayer.play(START_SOUND);
        while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
        while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
        //Serial.println(state);
        //delay(1000);
        push[WHITE] = 0;
      }  
    }

    break;
  case SENSING:
    
    if (sens){ // sense vibration sensor
      numVib += 1;
      myDFPlayer.loop(VIBRATIONS_SOUND);  
      if (numVib <4){ //
        lcd.setCursor(15, 0);
        lcd.print(String(numVib));
        lcd.setCursor(btnCursol[numBtnT], 1);
        state = VIBRATION;
        //Serial.println(state);      
      }else{ // game over        
        while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
        while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
        myDFPlayer.stop(); 
        sens = false;
        game_over();
        state = FINISH;
        //Serial.println(state);
      }
      break;
    }
    
    if (push[BLUE] || push[YELLOW] || push[RED]){
      if (isPushbuttonClicked(BLUE) == 2 || isPushbuttonClicked(YELLOW) == 2 || isPushbuttonClicked(RED) == 2){ // released the button
        state = BUTTON;
        Serial.println(state);
        break; 
      }          
    }

    if (push[GREEN]){
      if (isPushbuttonClicked(GREEN) == 1){
        if (numBtnT == 3 ){
          game_clear();        
        }else{
          game_over();   
        }
        state = FINISH;
        //Serial.println(state);
        //delay(1000);
        push[GREEN] = 0;
        break;  
      }      
    }

    if (push[WHITE]){
      if (isPushbuttonClicked(WHITE) == 1){
        numVib = 0; 
        numBtnF = 0;
        numBtnT = 0;
        lcd.setCursor(6, 0);
        lcd.print("  T0 F0 A0");
        lcd.setCursor(btnCursol[numBtnT], 1);
        myDFPlayer.play(START_SOUND);
        //delay(1000);
        push[WHITE] = 0;
        break;
      }
    }    
    break;
  case VIBRATION:
    sens = false;
    delay(500);
    if(sens){
      novib = 0;     
    }else{ 
      novib += 1;
      if(novib > 2){ // exit the VIBRATION state on condition of no vibration for 500ms x4 
        while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
        while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
        myDFPlayer.stop();
        novib = 0;
        sens = false;
        state = SENSING;
        //Serial.println(state);        
      }
    }    
    break;
  case BUTTON:
    if (numBtnT < 3 && push[order[currMode][numBtnT]] == true ){
      numBtnT += 1;
      lcd.setCursor(9, 0);
      lcd.print(String(numBtnT));
      lcd.setCursor(btnCursol[numBtnT], 1);
      myDFPlayer.play(BUTTON_TRUE_SOUND);
      while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
      while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
      //delay(300);
      state = SENSING;
      //Serial.println(state);
    }else{
      numBtnF += 1;
      lcd.setCursor(12, 0);
      lcd.print(String(numBtnF));
      lcd.setCursor(btnCursol[numBtnT], 1);
      myDFPlayer.play(BUTTON_FALSE_SOUND);
      while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
      while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
      //delay(300);
      if (numBtnF < 3){
        state = SENSING;
        //Serial.println(state);
      }else{ // 2 or more mistakes is game over. 
        game_over();
        state = FINISH;
        //Serial.println(state);
      }
    }
    //delay(1000);
    push[BLUE] = 0;
    push[YELLOW] = 0;
    push[RED] = 0;
    break;
  case FINISH:
    numVib = 0; 
    numBtnF = 0;
    numBtnT = 0;
    
    currMode = map(analogRead(PIN_PTM), 0, 4095, 0, 7);
    if (currMode != prevMode){
      state = WAITING;
      //Serial.println(state);
      break;
    }
    
    if (push[WHITE]){
      if (isPushbuttonClicked(WHITE) == 1){
        state = WAITING;
        prevMode = 10;
        //delay(1000);
        push[WHITE] = 0;
        break;
      }
    }
    break;
  default:
    // statements
    break;
  }
   
}

void push_btn_W(){
  push[WHITE] = true;
}

void push_btn_B(){
  push[BLUE] = true;
}

void push_btn_Y(){
  push[YELLOW] = true;
}

void push_btn_R(){
  push[RED] = true;
}

void push_btn_G(){
  push[GREEN] = true;
}

void sens_vib(){
  sens = true;
}


// game over 
void game_over(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAME OVER");
  lcd.setCursor(0, 1);
  lcd.print("TRY AGAIN!");
  lcd.noBlink();
  delay(500);
  myDFPlayer.play(GAMEOVER1_SOUND);
  while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
  while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
  myDFPlayer.play(GAMEOVER2_SOUND);
  while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
  while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
}

// game clear
void game_clear(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONGRATULATION!");
  lcd.noBlink();
  myDFPlayer.play(GAMECLEAR1_SOUND);
  while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
  while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
  //delay(2000);
  myDFPlayer.play(GAMECLEAR2_SOUND);
  while(digitalRead(PIN_DFP)==HIGH); // wait for starting the sound 
  while(digitalRead(PIN_DFP)==LOW); // wait for finishing the sound
}

/* check the push button state */
// 0: no pushed
// 1: pushed
// 2: released 
int isPushbuttonClicked(int i) {

    int state = digitalRead(PIN_BTN[i]);
    if (state != lastButtonState[i]) { // reset timer
        lastDebounceTime[i] = millis();
    }
  
    if ((millis() - lastDebounceTime[i]) > debounceWindow) {
        if (state != buttonState[i]) {
            buttonState[i] = state;
            if (buttonState[i] == LOW) { // pushed
                lastButtonState[i] = state;
                return 1;
            }
            if (buttonState[i] == HIGH) { // released
                lastButtonState[i] = state;
                return 2;
            }
        }
    }
    lastButtonState[i] = state;
    return 0;
}
