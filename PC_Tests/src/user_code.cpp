
// If you write this, you can use drawBmpFile / drawJpgFile / drawPngFile
// #include <stdio.h>

// If you write this, you can use drawBmpUrl / drawJpgUrl / drawPngUrl ( for
// Windows ) #include <windows.h> #include <winhttp.h> #pragma comment (lib,
// "winhttp.lib")

#include "gen.h"
#include "giac.h"
#include "global.h"
#include "minigiac.hpp"
#include "usual.h"
#include <dirent.h>
#include <stdint.h>

#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

#include "global.hpp"
#include "terminal.hpp"
#include "u8g2_replacer.hpp"

#include <iostream>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "giac_ast.hpp"

#include "giac_cursor.hpp"



LGFX lcd(320, 240);
LGFX lcd2(135, 240);

lgfx::LGFX_Sprite canvas(&lcd);

MathRenderer::AstCursor cursor;

const lgfx::U8g2font font7x13symbols(u8g2_font_7x13_m_symbols);
const lgfx::U8g2font font7x13bold(u8g2_font_7x13B_tf);
const lgfx::U8g2font font5x8normal(u8g2_font_5x8_tf);
const lgfx::U8g2font font9x15symbols(u8g2_font_9x15_m_symbols);

int32_t target_x = 160 * 256;
int32_t target_y = 120 * 256;
int32_t current_x = 0;
int32_t current_y = 0;
int32_t add_x = 0;
int32_t add_y = 0;

Terminal term;

giac::context ct; // Giac context (assuming giac namespace)
giac::gen pretty_result;
bool has_pretty_result = true;

int i = 0;
int n = 0;

// Custom key codes for special keys
enum KeyCode {
  KEY_NONE = 0,
  KEY_UP = 1000,
  KEY_DOWN = 1001,
  KEY_RIGHT = 1002,
  KEY_LEFT = 1003
};

int readKeyNonBlockingSDL() {
  SDL_Event events[20];

  SDL_PumpEvents();

  int count =
      SDL_PeepEvents(events, 20, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);

  if (count > 0 && events[0].type == SDL_KEYDOWN) {
    SDL_Keycode sym = events[0].key.keysym.sym;

    // Check if Left or Right Shift is currently held down
    SDL_Keymod mods = SDL_GetModState();
    bool is_shifted = (mods & KMOD_SHIFT) != 0;

    // Flush immediately to prevent character duplication
    SDL_FlushEvent(SDL_KEYDOWN);

    // Map special keys
    switch (sym) {
    case SDLK_UP:
      return KEY_UP;
    case SDLK_DOWN:
      return KEY_DOWN;
    case SDLK_LEFT:
      return KEY_LEFT;
    case SDLK_RIGHT:
      return KEY_RIGHT;
    default:
      break;
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
        case '1':
          return '!';
        case '2':
          return '@';
        case '3':
          return '#';
        case '4':
          return '$';
        case '5':
          return '%';
        case '6':
          return '^';
        case '7':
          return '&';
        case '8':
          return '*';
        case '9':
          return '(';
        case '0':
          return ')';
        case '-':
          return '_';
        case '=':
          return '+';
        case '[':
          return '{';
        case ']':
          return '}';
        case '\\':
          return '|';
        case ';':
          return ':';
        case '\'':
          return '"';
        case ',':
          return '<';
        case '.':
          return '>';
        case '/':
          return '?';
        case '`':
          return '~';
        default:
          break;
        }
      }
      return static_cast<int>(sym);
    }
  }

  return KEY_NONE;
}

bool isAscii(int val) { return val >= 32 && val <= 126; }

void setup() {
  // lcd2.init();
  //  -------------------- Initialise LovyanGFX --------------------
  lcd.init(); // this includes SDL_Init, etc.
  lcd.setRotation(0);
  lcd.fillScreen(TFT_BLACK);

  canvas.setColorDepth(lgfx::palette_4bit);
  canvas.createSprite(320, 240);
  canvas.fillScreen(TFT_BLACK);

  term.init(&canvas, 320, 120, &font7x13symbols, &font7x13bold, TFT_WHITE,
            TFT_BLACK);
  term.addChar(0, Terminal::FLAGS_IMMUNE);
  term.addChar(' ', Terminal::FLAGS_IMMUNE);

  giac::vecteur sumTest;
  sumTest.push_back(giac::gen(1));
  sumTest.push_back(giac::gen(2));
  sumTest.push_back(giac::gen(3));
  sumTest.push_back(giac::gen(-4));
  sumTest.push_back(giac::gen(5));

  giac::gen testA = giac::symbolic(giac::at_plus, sumTest);
  std::cout << "testA: " << testA.print() << std::endl;
  print_giac_ast_iterative(testA);
  giac::gen testB = giac::gen("1+(3+5*x/sin(3*x))/(1/1/1)", &ct);
  testB = convert_inv_to_div(testB);
  pretty_result = testB;
  std::cout << "testB: " << testB.print() << std::endl;
  print_giac_ast_iterative(testB);
}

using namespace std;
using namespace giac;

void loop() {
  canvas.fillSprite(TFT_BLACK);

  int key = readKeyNonBlockingSDL();
  if (isAscii(key))
    term.addChar(key & 0xFF);
  if (key == 13 || key == 10) { // Enter control char
    term.enter();

    char *buffer = term.getCurrentInput();
    char *p = buffer;
    term.addChar('g', Terminal::FLAGS_IMMUNE);
    term.addChar('i', Terminal::FLAGS_IMMUNE);
    term.addChar('a', Terminal::FLAGS_IMMUNE);
    term.addChar('c', Terminal::FLAGS_IMMUNE);
    term.addChar(':', Terminal::FLAGS_IMMUNE);
    term.addChar(' ', Terminal::FLAGS_IMMUNE);
    // while(p && *p){
    //   term.addChar(*p);
    //   p++;
    // }

    try {
      giac::gen g = giac::gen(buffer, &ct);
      giac::gen result = giac::eval(g, 1, &ct);
      // giac::gen amogus = giac::symbolic(giac::at_sin,
      // giac::symbolic(giac::at_plus, giac::symbolic(giac::at_pow,
      // giac::identificateur("x"), giac::gen(2)), giac::gen(3)));
      std::cout << "input: " << std::endl;
      print_giac_ast_iterative(g);
      std::cout << "output: " << std::endl;
      result = convert_inv_to_div(result);
      print_giac_ast_iterative(result);
      pretty_result = convert_inv_to_div(g);
      cursor.setRoot(&pretty_result);
      has_pretty_result = true;
    } catch (const runtime_error &err) {
      for (char c : "ERROR: ") {
        if (c != '\0')
          term.addChar(c);
      }

      // 2. Send error message contents
      for (const char *p = err.what(); *p != '\0'; ++p) {
        if (*p == 13 || *p == 10)
          term.enter();
        else
          term.addChar(*p);
      }
    }

    calcemistFree(buffer);
    term.enter();
    term.addChar(0, Terminal::FLAGS_IMMUNE);
    term.addChar(' ', Terminal::FLAGS_IMMUNE);
  }

  if (key == 8) // Backspace control char
    term.backspace();

  if (key == KEY_LEFT){
    term.moveCursorLeft();
    //MathRenderer::move_cursor(Direction::LEFT);
    cursor.move(Direction::LEFT);
  }
  if (key == KEY_RIGHT){
    term.moveCursorRight();
    //MathRenderer::move_cursor(Direction::RIGHT);
    cursor.move(Direction::RIGHT);
  }
  if (key == KEY_UP)
    term.scroll(-1);
  if (key == KEY_DOWN)
    term.scroll(1);




  term.render(0, 120);

  if (has_pretty_result) {
    MathFonts fonts{&font9x15symbols, &font7x13symbols, &font5x8normal};
    render_giac_ast(&canvas, fonts, pretty_result, 8, 8);
  }

  canvas.pushSprite(0, 0);

  // ---- 5. Small delay to prevent 100% CPU ----
  // lgfx::delay(1); // or SDL_Delay(1)
  SDL_Delay(1);
}
