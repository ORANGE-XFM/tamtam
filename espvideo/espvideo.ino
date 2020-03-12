#define double_buffer
#include <PxMatrix.h>
#include <FS.h>

#include "gifdec.h"

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
//#define P_D 12
//#define P_E 0
#define P_OE 2
// Pins for LED MATRIX

PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);

// ISR for display refresh
void display_updater()
{
  display.display(66);
}


struct gd_GIF *gif;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.printf("Hello !\n");
  SPIFFS.begin();
  Serial.println("File System Initialized");

  gif = gd_open_gif("/video.gif");
  
  display.begin(8);
  display.flushDisplay();
  display.setTextWrap(false);

  display_ticker.attach(0.002, display_updater);

}

uint8_t buffer[32*16*3];
void loop() {
  int res = gd_get_frame(gif);
  if (!res) gd_rewind(gif);

  gd_render_frame(gif, buffer);
  delay(30);
  display.drawRGBBitmap(0,0,(uint16_t *)buffer,28,16);
  display.showBuffer();
}
