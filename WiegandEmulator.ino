#include <Keypad.h>
#include <ctype.h>
#include <SPI.h>
#include <Wire.h>
// https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_GFX.h>
// https://github.com/adafruit/Adafruit_SSD1306
#include <Adafruit_SSD1306.h>

#define D_Zero 2
#define D_One  3
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Keypad Setup
const byte ROWS = 4; 
const byte COLS = 4; 
const char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = { 5, 6, 7, 8 }; 
byte colPins[COLS] = {9, 10, 11, 12};

// Max cardnumber length is 10 digits.
char CardNumber[10];
// Initialize max_number with default value for 26-bit cards.
unsigned long max_number = 65536;
byte card_type = 0;
byte data_count = 0;

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void setup(){
  pinMode(D_Zero, OUTPUT);   
  pinMode(D_One, OUTPUT);
  // Wiegand lines are pulled low when active.
  digitalWrite(D_Zero, HIGH);
  digitalWrite(D_One, HIGH);
  keypad.addEventListener(keypadEvent);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(500);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Wiegand");
  display.setCursor(0, 15);
  display.println("Emulator");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("26-bit");
  display.display();
}

void loop(){
  char key = keypad.getKey();
  if (isdigit(key)) {
    CardNumber[data_count] = key;
    // Check to see if the card number is larger than the max for the card type.
    if(atol(CardNumber) > max_number) {
      CardNumber[data_count] = 0;
    } else {
      data_count++;
    }
    display.setTextSize(3);
    display.setCursor(0,10);
    display.println(CardNumber);
    display.display();
  } 

  switch(card_type) {
    case 0: // 26-bit
      display.setTextSize(1);
      display.setCursor(0,0);
      display.println("26-bit");
      display.display();
      max_number = 65535;
      break;
    case 1: // 35-bit  
      display.setTextSize(1);
      display.setCursor(0,0);
      display.println("35-bit");
      display.display();
      max_number = 1048575;
      break;
    case 2: // 37-bit
      display.setTextSize(1);
      display.setCursor(0,0);
      display.println("37-bit");
      display.display();
      max_number = 524287;
      break;
    case 3: // 48-bit
      display.setTextSize(1);
      display.setCursor(0,0);
      display.println("48-bit");
      display.display();
      max_number = 8388607;
      break;
  }
}

void keypadEvent(KeypadEvent key){
    if(keypad.getState() == PRESSED){
      display.clearDisplay();
      switch(key) {
        case 'A': // 26-bit Card Format
          clearData();
          card_type = 0;
          break;
        case 'B': // 35-bit Card Format
          clearData();
          card_type = 1;
          break;
        case 'C': // 37-bit Card Format
          clearData();
          card_type = 2;
          break;
        case 'D': // 48-bit Card Format
          clearData();
          card_type = 3;
          break;
        case '#': // Enter
          switch(card_type) {
            case 0:
              twentySixBit();
              break;
            case 1:
              thirtyFiveBit();
              break;
            case 2:
              thirtySevenBit();
              break;
            case 3:
              fortyEightBit();
              break;
          }
          clearData();
          display.setTextSize(3);
          display.setCursor(0,10);
          display.println(F(" SWIPE"));
          display.display();
          delay(1000);
          display.clearDisplay();
          break;
        case '*': // Backspace
          if(data_count > 0) {
            data_count--;
            CardNumber[data_count] = 0;
            display.setTextSize(3);
            display.setCursor(0,10);
            display.println(CardNumber);
            display.display(); 
          }
          break;
      }
    }
}

// * key to erase.
void clearData(){
  while(data_count !=0){
    CardNumber[data_count--] = 0; 
  }
  return;
}

/*
 * Wiegand Signal Handling
 * Pull the line low for 50ms to signal a 0 or 1.
 */
 
void writeD0(){
  digitalWrite(D_Zero, LOW);
  delayMicroseconds(50);
  digitalWrite(D_Zero, HIGH);
}
void writeD1(){
  digitalWrite(D_One, LOW);
  delayMicroseconds(50);
  digitalWrite(D_One, HIGH);
}

/* 
 * Card Formats
 * Only card number bits are sent, 0's are sent for the facility code and parity bits. 
 * I don't need either of these currently but it wouldn't be much to add them.
 */

// Card number 0-65536.
// PFFFFFFFFCCCCCCCCCCCCCCCCP
void twentySixBit(){
  unsigned long mask = 0x8000;
  unsigned long number = atol(CardNumber);
  // Send 9 0's for parity and facility code.
  for(int i=0; i<9; i++) {
    writeD0();
    delay(2);
  }
  for(int i=0; i<16; i++){
    if(mask & number) writeD1(); else writeD0();
    mask = mask >> 1;
    delay(2);
  }
  // Send a 0 for the final parity bit.
  writeD0();
}

// Card number 0-1048575.
// PPFFFFFFFFFFFFCCCCCCCCCCCCCCCCCCCCP
void thirtyFiveBit(){
  unsigned long mask = 0x80000;
  unsigned long number = atol(CardNumber);
  // Send 14 0's for parity and facility code.
  for(int i=0; i<14; i++) {
    writeD0();
    delay(2);
  }
  for(int i=0; i<20; i++){
    if(mask & number) writeD1(); else writeD0();
    mask = mask >> 1;
    delay(2);
  }
  // Send a 0 for the final parity bit.
  writeD0();
}

// Card number 0-524287
// PFFFFFFFFFFFFFFFFCCCCCCCCCCCCCCCCCCCP
void thirtySevenBit() {
  unsigned long mask = 0x40000;
  unsigned long number = atol(CardNumber);
  for(int i=0; i<17; i++) {
    writeD0();
    delay(2);
  }
  for(int i=0; i<19; i++) {
    if(mask & number) writeD1(); else writeD0();
    mask = mask >> 1;
    delay(2);
  }
  writeD0();
}

// Card number 0-8388607
// PPAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBP
void fortyEightBit() {
  unsigned long mask = 0x400000;
  unsigned long number = atol(CardNumber);
  for(int i=0; i<24; i++) {
    writeD0();
    delay(2);
  }
  for(int i=0; i<23; i++) {
    if(mask & number) writeD1(); else writeD0();
    mask = mask >> 1;
    delay(2);
  }
  writeD0();
}
