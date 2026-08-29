
// If you write this, you can use drawBmpFile / drawJpgFile / drawPngFile
// #include <stdio.h>

// If you write this, you can use drawBmpUrl / drawJpgUrl / drawPngUrl ( for
// Windows ) #include <windows.h> #include <winhttp.h> #pragma comment (lib,
// "winhttp.lib")

#include "giac.h"
#include "minigiac.hpp"
#include <stdint.h>

#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

#include "u8g2_replacer.hpp"
#include "terminal.hpp"


#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <iostream>



LGFX lcd(320, 240);
LGFX lcd2(135, 240);

lgfx::LGFX_Sprite canvas(&lcd);

const lgfx::U8g2font font7x13symbols( u8g2_font_7x13_m_symbols );



int32_t target_x = 160 * 256;
int32_t target_y = 120 * 256;
int32_t current_x = 0;
int32_t current_y = 0;
int32_t add_x = 0;
int32_t add_y = 0;

Terminal term;

giac::context ct; // Giac context (assuming giac namespace)

int i = 0;
int n = 0;



// Custom key codes for special keys
enum KeyCode {
    KEY_NONE  = 0,
    KEY_UP    = 1000,
    KEY_DOWN  = 1001,
    KEY_RIGHT = 1002,
    KEY_LEFT  = 1003
};


int readKeyNonBlockingSDL() {
    SDL_Event events[20];
    
    SDL_PumpEvents();

    int count = SDL_PeepEvents(events, 20, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);

    if (count > 0 && events[0].type == SDL_KEYDOWN) {
        SDL_Keycode sym = events[0].key.keysym.sym;

        // Check if Left or Right Shift is currently held down
        SDL_Keymod mods = SDL_GetModState();
        bool is_shifted = (mods & KMOD_SHIFT) != 0;

        // Flush immediately to prevent character duplication
        SDL_FlushEvent(SDL_KEYDOWN);

        // Map special keys
        switch (sym) {
            case SDLK_UP:    return KEY_UP;
            case SDLK_DOWN:  return KEY_DOWN;
            case SDLK_LEFT:  return KEY_LEFT;
            case SDLK_RIGHT: return KEY_RIGHT;
            default: break;
        }

        // Return control keys (Enter / Backspace / Linefeed)
        if (sym == 10 || sym == 8 || sym == 13) {
            return static_cast<int>(sym);
        }

        // Handle Shift conversions for printable ASCII
        if (sym >= 32 && sym <= 126) {
            if (is_shifted) {
                // Lowercase letters to Uppercase
                if (sym >= 'a' && sym <= 'z') {
                    return sym - 32;
                }

                // Numbers and punctuation shift map (Standard US Layout)
                switch (sym) {
                    case '1': return '!';
                    case '2': return '@';
                    case '3': return '#';
                    case '4': return '$';
                    case '5': return '%';
                    case '6': return '^';
                    case '7': return '&';
                    case '8': return '*';
                    case '9': return '(';
                    case '0': return ')';
                    case '-': return '_';
                    case '=': return '+';
                    case '[': return '{';
                    case ']': return '}';
                    case '\\': return '|';
                    case ';': return ':';
                    case '\'': return '"';
                    case ',': return '<';
                    case '.': return '>';
                    case '/': return '?';
                    case '`': return '~';
                    default: break;
                }
            }
            return static_cast<int>(sym);
        }
    }

    return KEY_NONE;
}


bool isAscii(int val) {
    return val >= 32 && val <= 126;
}


void setup() {
  // lcd2.init();
  //  -------------------- Initialise LovyanGFX --------------------
  lcd.init(); // this includes SDL_Init, etc.
  lcd.setRotation(0);
  lcd.fillScreen(TFT_BLACK);

  canvas.setColorDepth(lgfx::palette_4bit);
  canvas.createSprite(320, 240);
  canvas.fillScreen(TFT_BLACK);

  term.init(&canvas, 320, 240, &font7x13symbols, TFT_WHITE, TFT_BLACK);
}

using namespace std;
using namespace giac;



void loop() {

  // ---- 2. Update drawing (your original loop logic) ----
  //++i;
  //lcd.fillCircle(current_x >> 8, current_y >> 8, 5, i);
  //current_x += add_x;
  //current_y += add_y;
  //add_x += (current_x < target_x) ? 1 : -1;
  //add_y += (current_y < target_y) ? 1 : -1;
//
  //// ---- 3. Touch input ----
  //lgfx::touch_point_t new_tp;
  //if (lcd.getTouch(&new_tp)) {
  //  target_x = new_tp.x * 256;
  //  target_y = new_tp.y * 256;
  //  lcd.drawCircle(new_tp.x, new_tp.y, 5, TFT_WHITE);
  //}
//
  //// ---- 4. Console input (non‑blocking) ----
  //string line;
  //if (readLineNonBlocking(line)) {
  //  cout << n << ">> " << line << endl;
  //  cout << n++ << "<< ";
  //  try {
  //    giac::gen g = giac::gen(line, &ct);
  //    giac::gen result = giac::eval(g, 1, &ct);
  //    cout << result << endl;
  //  } catch (const runtime_error &err) {
  //    cout << "ERROR: " << err.what() << endl;
  //  }
  //}
  canvas.fillSprite(TFT_BLACK);
  
  int key = readKeyNonBlockingSDL();
  if(isAscii(key))
    term.addChar(key & 0xFF);
  if(key == 13 || key == 10) // Enter control char
    term.enter();
  if(key == 8) //Backspace control char
    term.backspace();
  
  term.render(0,0);

  canvas.pushSprite(0,0);

  // ---- 5. Small delay to prevent 100% CPU ----
  //lgfx::delay(1); // or SDL_Delay(1)
  SDL_Delay(1);
}