/*
 * IRremoteESP8266: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * This is very simple teaching code to show you how to use the library.
 * If you are trying to decode your Infra-Red remote(s) for later replay,
 * use the IRrecvDumpV2.ino example code instead of this.
 * An IR detector/demodulator must be connected to the input kRecvPin.
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 * Example circuit diagram:
 *  https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-receiving
 * Changes:
 *   Version 0.2 June, 2017
 *     Changed GPIO pin to the same as other examples.
 *     Used our own method for printing a uint64_t.
 *     Changed the baud rate to 115200.
 *   Version 0.1 Sept, 2015
 *     Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009
 */

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SPI.h> // not used *here* but needed for library introspection
#include <PxMatrix.h>
#include <Ticker.h>

// An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
// board).
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
const uint16_t kRecvPin = D1;

IRrecv irrecv(kRecvPin);
// Setup LED panel
Ticker display_ticker;
#define P_LAT D0
#define P_A D6
#define P_B D3
#define P_C D8
// #define P_D 12
// #define P_E 0
#define P_OE D4

#define matrix_width 32
#define matrix_height 16

PxMATRIX display(matrix_width,matrix_height,P_LAT, P_OE,P_A,P_B,P_C);

decode_results results;

uint16_t textColor = display.color565(255, 0, 0);
uint16_t crossColor = display.color565(0, 255, 0);

// ISR for display refresh

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time=20; //10-50 is usually fine

void display_updater()
{
  display.display(display_draw_time);
}

void display_update_enable(bool is_enable)
{

  if (is_enable)
    display_ticker.attach(0.002, display_updater);
  else
    display_ticker.detach();
}

enum Direction {DIR_NONE=0, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};

int x,y;
Direction dir;

void setup() {

  // setup IR
  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);

  // setup display 
  // Define your display layout here, e.g. 1/8 step, and optional SPI pins begin(row_pattern, CLK, MOSI, MISO, SS)
  display.begin(8);

  // Intro
  display.setFastUpdate(true);
  display.clearDisplay();
  display.setTextColor(textColor);
  display.setCursor(2,0);
  display.print("IR");
  display.setCursor(2,8);
  display.print("Time");
  display_update_enable(true);

  delay(3000);
  display.clearDisplay();

  x=matrix_width/2;
  y=matrix_height/2;
}

void update_display(int x, int y) {
    display.clearDisplay();
    display.drawLine(x-2,y,x+2,y,crossColor);
    display.drawLine(x,y-2,x,y+2,crossColor);
}

void loop() {
  if (irrecv.decode(&results)) {
    // print() & println() can't handle printing long longs. (uint64_t)
    serialPrintUint64(results.value, HEX);
    Serial.println("");
    Serial.print("Protocol: ");
    Serial.println(results.decode_type);

    // update display 
    switch (results.value) {
      case 0xFFC03F: dir=DIR_UP; break;
      case 0xFF40BF: dir=DIR_DOWN; break;
      case 0xFF708F: dir=DIR_LEFT; break;
      case 0xFF58A7: dir=DIR_RIGHT; break;
      case 0xFFFFFFFFFFFFFFFFUL: 
        // *don't* reset dir
        Serial.println("again"); 
        break;
      //case 0xFFE01F: Serial.println("ok"); break;
      default: 
        dir=DIR_NONE;
    }

    switch(dir) {
      case DIR_UP:    y--; if (y<0)             y=0;            break;
      case DIR_DOWN:  y++; if (y>matrix_height) y=matrix_width; break;
      case DIR_LEFT:  x--; if (x<0)             x=0;            break;
      case DIR_RIGHT: x++; if (x>matrix_width)  x=matrix_width; break;
    }

    update_display(x,y);
    irrecv.resume();  // Receive the next value
  }
  delay(100);
}
