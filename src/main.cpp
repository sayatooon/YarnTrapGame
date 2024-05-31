#include <Arduino.h>

#include <DFRobotDFPlayerMini.h>
#include <LiquidCrystal_I2C.h>

// state
#define WAITING 0
#define SENSING 1
#define VIBRATION 2
#define BUTTON 3
#define FINISH 4
// mode
#define SPADE 0
#define HEART 1
#define DIAMOND 2
#define CLUB 3
#define START 4
#define MOON 5
#define CROWN 6
#define FREE 7
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
bool buttonState[] = {false, false, false, false, false}, buttonON[] = {false, false, false, false, false}; // button/sensor state
//unsigned long prevTime_B[] = {0, 0, 0, 0, 0},  prevTime_S = 0;
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

  // LCD setting
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("LCD connect.    ");

  //speaker setting
  myDFPlayer.begin(HardwareSerial);
  if (!myDFPlayer.begin(HardwareSerial)) {  //Use softwareSerial to communicate with mp3.
    lcd.setCursor(0, 1);
    lcd.print("Speaker fail.   ");
  }else{
    lcd.setCursor(0, 1);
    lcd.print("Speaker connect.");
    myDFPlayer.volume(30); //Set volume value. From 0 to 30  
  }
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
    
      //if (currMode == FREE){
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
    
    //check_button(WHITE);
    //if (buttonON[WHITE]==true){
    if (push[WHITE]){
      myDFPlayer.play(START_SOUND);
      numVib = 0; 
      numBtnF = 0;
      numBtnT = 0;
      lcd.setCursor(btnCursol[numBtnT], 1);
      lcd.blink();
      state = SENSING;
      //Serial.println(state);
      delay(1000);
      push[WHITE] = 0;
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
        sens = false;
        myDFPlayer.stop();
        game_over();
        state = FINISH;
        //Serial.println(state);
      }

      break;
    }
    
    // check_button(BLUE);
    // check_button(YELLOW);
    // check_button(RED);   
    // if (buttonON[BLUE] || buttonON[YELLOW] || buttonON[RED]){ // pushed color buttons
    if (push[BLUE] || push[YELLOW] || push[RED]){
      state = BUTTON;
      Serial.println(state);
      break;      
     }

    // check_button(GREEN);
    // if (buttonON[GREEN]){ // pushed the goal button
    if (push[GREEN]){
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

    // check_button(WHITE);
    // if (buttonON[WHITE]){ // pushed the start button -restart
    if (push[WHITE]){
      numVib = 0; 
      numBtnF = 0;
      numBtnT = 0;
      lcd.setCursor(6, 0);
      lcd.print("  T0 F0 A0");
      lcd.setCursor(btnCursol[numBtnT], 1);
      myDFPlayer.play(START_SOUND);
      delay(1000);
      push[WHITE] = 0;
      break;
    }    
    break;
  case VIBRATION:
    if(sens){
      novib = 0;      
      
    }else{
      novib += 1;
      if(novib > 2){
        myDFPlayer.stop();
        state = SENSING;
        //Serial.println(state);
        novib = 0;
        sens = false;
      }      
    }
    sens = false;
    delay(500);
    break;
  case BUTTON:
    //if (buttonON[order[currMode][numBtnT]] == true ){
    if (push[order[currMode][numBtnT]] == true ){
      numBtnT += 1;
      lcd.setCursor(9, 0);
      lcd.print(String(numBtnT));
      lcd.setCursor(btnCursol[numBtnT], 1);
      myDFPlayer.play(BUTTON_TRUE_SOUND);
      delay(300);
      state = SENSING;
      //Serial.println(state);
    }else{
      numBtnF += 1;
      lcd.setCursor(12, 0);
      lcd.print(String(numBtnF));
      lcd.setCursor(btnCursol[numBtnT], 1);
      myDFPlayer.play(BUTTON_FALSE_SOUND);
      delay(300);
      if (numBtnF < 3){
        state = SENSING;
        //Serial.println(state);
      }else{ // 2 or more mistakes is game over. 
        game_over();
        state = FINISH;
        //Serial.println(state);
      }
    }
    delay(1000);
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
    
    // check_button(WHITE);
    // if (buttonON[WHITE]==true){
    if (push[WHITE]){
      state = WAITING;
      prevMode = 10;
      delay(1000);
      push[WHITE] = 0;
      break;
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

// update buttonState 
// void check_button(int i){
//   buttonON[i] = false;
//   if(buttonState[i] == true && millis()-prevTime_B[i] > 1000 && digitalRead(PIN_BTN[i])){
//     buttonState[i] = false;
//     push[i] = false;    
//   }
//   if (push[i]){          
      
//       if(buttonState[i] == false){
//         buttonState[i] = true;
//         prevTime_B[i] = millis();
//         buttonON[i] = true;        
//       }
//       push[i] = false;
//     }
// }

// game over 
void game_over(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAME OVER");
  lcd.setCursor(0, 1);
  lcd.print("TRY AGAIN!");
  lcd.noBlink();
  //delay(1000);
  myDFPlayer.play(GAMEOVER1_SOUND);
  delay(3000);
  myDFPlayer.play(GAMEOVER2_SOUND);
}

// game clear
void game_clear(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONGRATULATION!");
  lcd.noBlink();
  myDFPlayer.play(GAMECLEAR1_SOUND);
  delay(2000);
  myDFPlayer.play(GAMECLEAR2_SOUND);
}
