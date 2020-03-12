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

raycaster  :

 * Copyright (c) 2004-2007, Lode Vandevenne
 * Copyright (c) 2015, Makapuf
 * Copyright (c) 2020, Makapuf
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 *
         * * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
         * * Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


 */
#include <math.h>
#include <stdint.h>
#include <stdlib.h> // abs
#include <string.h>

#include "terrain.h" // includes data

#define H_PIXELS 32
#define V_PIXELS 16

#include <Arduino.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <PxMatrix.h>
#include <SPI.h> // not used *here* but needed for library introspection
#include <Ticker.h>

// An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
// board).
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
const uint16_t kRecvPin = D1;

#define INTRO

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

PxMATRIX display(H_PIXELS, V_PIXELS, P_LAT, P_OE, P_A, P_B, P_C);

decode_results results;

uint16_t textColor = display.color565(255, 0, 0);
uint16_t crossColor = display.color565(0, 255, 0);

// ISR for display refresh

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time = 20; // 10-50 is usually fine

void display_updater() { display.display(display_draw_time); }

enum Direction { DIR_NONE = 0, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

float posX = 22.f, posY = 12.f;  // x and y start position
float dirX = -1.f, dirY = 0.f;   // initial direction vector
float planeX = 0, planeY = 0.66; // the 2d raycaster version of camera plane

void setup() {

  // setup IR
  Serial.begin(115200);
  irrecv.enableIRIn(); // Start the receiver
  while (!Serial)          // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);

  // setup display
  // Define your display layout here, e.g. 1/8 step, and optional SPI pins
  // begin(row_pattern, CLK, MOSI, MISO, SS)
  display.begin(8);

  // Intro
  display.setFastUpdate(true);
  // enable display
  display_ticker.attach(0.002, display_updater);

#ifdef INTRO
  display.clearDisplay();
  display.setTextColor(textColor);
  display.setCursor(2, 0);
  display.print("LED");
  display.setCursor(2, 8);
  display.print("Stein");
  delay(3000);
#endif

  display.clearDisplay();
}

void update_display() {
  display.clearDisplay();

  display.drawLine(posX - 1, posY, posX + 1, posY, crossColor);
  display.drawLine(posX, posY - 1, posX, posY + 1, crossColor);

  for (int x = 0; x < 16 /*H_PIXELS*/; x++) {
    // calculate ray position and direction
    float cameraX =
        2.f * x / (float)(H_PIXELS)-1.f; // x-coordinate in camera space
    float rayPosX = posX;
    float rayPosY = posY;
    float rayDirX = dirX + planeX * cameraX;
    float rayDirY = dirY + planeY * cameraX;
    // which box of the map we're in
    int mapX = (int)(rayPosX);
    int mapY = (int)(rayPosY);

    // length of ray from current position to next x or y-side
    float sideDistX;
    float sideDistY;

    // length of ray from one x or y-side to next x or y-side
    float deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
    float deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));
    float perpWallDist;

    // what direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    int hit = 0;  // was there a wall hit?
    int side = 0; // was a NS or a EW wall hit?
    // calculate step and initial sideDist
    if (rayDirX < 0) {
      stepX = -1;
      sideDistX = (rayPosX - mapX) * deltaDistX;
    } else {
      stepX = 1;
      sideDistX = (mapX + 1.0f - rayPosX) * deltaDistX;
    }

    if (rayDirY < 0) {
      stepY = -1;
      sideDistY = (rayPosY - mapY) * deltaDistY;
    } else {
      stepY = 1;
      sideDistY = (mapY + 1.0f - rayPosY) * deltaDistY;
    }

    // perform DDA
    while (!hit) {
      // jump to next map square, OR in x-direction, OR in y-direction
      if (sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      } else {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }
      // Check if ray has hit a wall
      if (worldMap[mapX][mapY] > 0)
        hit = 1;
    }

    // Calculate distance projected on camera direction (oblique distance will
    // give fisheye effect!)
    if (side == 0)
      perpWallDist = fabsf((mapX - rayPosX + (1 - stepX) / 2) / rayDirX);
    else
      perpWallDist = fabsf((mapY - rayPosY + (1 - stepY) / 2) / rayDirY);

    // Calculate height of line to draw on screen
    int lineHeight = abs((int)(V_PIXELS / perpWallDist));

    // calculate pixel to fill in current stripe (will get other symmetrically
    // for now. no need to clamp)
    int drawStart = -lineHeight / 2 + V_PIXELS / 2;
    if (drawStart < 0)
      drawStart = 0;
    int drawEnd = lineHeight / 2 + V_PIXELS / 2;
    if (drawEnd >= V_PIXELS)
      drawEnd = V_PIXELS - 1;

    // choose wall color
    uint16_t color = palette[worldMap[mapX][mapY]];

    // give x and y sides different brightness (? use a second palette ?)
    if (side)
      color &= 0b111001110011100;

    for (int i = drawStart; i < drawEnd; i++) {
      display.drawPixel(x, i, color);
    }
  } // for x
}

void handle_input() {

  static Direction dir;

  switch (results.value) {
  case 0xFFC03F:
    dir = DIR_UP;
    break;
  case 0xFF40BF:
    dir = DIR_DOWN;
    break;
  case 0xFF708F:
    dir = DIR_LEFT;
    break;
  case 0xFF58A7:
    dir = DIR_RIGHT;
    break;
  case 0xFFFFFFFFFFFFFFFFUL:
    // *don't* reset dir
    break;
  // case 0xFFE01F: Serial.println("ok"); break;
  default:
    dir = DIR_NONE;
  }

  const char *dirs[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT"};
  Serial.println(dirs[dir]);

  // speed modifiers
  // asserts framerate of 60 fps, constant.
  float moveSpeed =
      0.5f; // 5.0f / 60.f; //the constant value is in squares/second
  float rotSpeed =
      0.1f; // 3.0f /60.f ; //the constant value is in radians/second
  float oldDirX, oldPlaneX;

  switch (dir) {
  case DIR_UP:
    if (worldMap[(int)(posX + dirX * moveSpeed)][(int)(posY)] == 0)
      posX += dirX * moveSpeed;
    if (worldMap[(int)(posX)][(int)(posY + dirY * moveSpeed)] == 0)
      posY += dirY * moveSpeed;
    break;

  case DIR_DOWN:
    if (worldMap[(int)(posX - dirX * moveSpeed)][(int)(posY)] == 0)
      posX -= dirX * moveSpeed;
    if (worldMap[(int)(posX)][(int)(posY - dirY * moveSpeed)] == 0)
      posY -= dirY * moveSpeed;
    break;

  case DIR_RIGHT:
    // both camera direction and camera plane must be rotated
    oldDirX = dirX;
    dirX = dirX * cosf(-rotSpeed) - dirY * sinf(-rotSpeed);
    dirY = oldDirX * sinf(-rotSpeed) + dirY * cosf(-rotSpeed);
    oldPlaneX = planeX;
    planeX = planeX * cosf(-rotSpeed) - planeY * sinf(-rotSpeed);
    planeY = oldPlaneX * sinf(-rotSpeed) + planeY * cosf(-rotSpeed);
    break;

  case DIR_LEFT:
    // both camera direction and camera plane must be rotated
    oldDirX = dirX;
    dirX = dirX * cosf(rotSpeed) - dirY * sinf(rotSpeed);
    dirY = oldDirX * sinf(rotSpeed) + dirY * cosf(rotSpeed);
    oldPlaneX = planeX;
    planeX = planeX * cosf(rotSpeed) - planeY * sinf(rotSpeed);
    planeY = oldPlaneX * sinf(rotSpeed) + planeY * cosf(rotSpeed);
    break;

  case DIR_NONE:
    break;
  }
}

// DBLBUFF ! speed draws ?

void loop() {
  if (irrecv.decode(&results)) {
    // serialPrintUint64(results.value, HEX);
    // Serial.println("");
    handle_input();
    irrecv.resume(); // Receive the next value
  }
  delay(100);
  update_display();
}
