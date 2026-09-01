#pragma once
#include <cstdint>
#include <stdint.h>
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

class Terminal {
public:
  Terminal();
  ~Terminal();

  void init(lgfx::LGFXBase *sprite, uint16_t width, uint16_t height,
            const lgfx::U8g2font *font, const lgfx::U8g2font *bfont, uint16_t f_color, uint16_t b_color);
  void deinit();
  void addChar(char c);
  void addChar(char c, uint8_t flags);
  int getCursorIndex();
  void setCursor(int idx);
  void moveCursorLeft();
  void moveCursorRight();
  void backspace();
  void enter();
  char *getCurrentInput();
  void render(uint16_t x, uint16_t y);
  void scroll(int8_t amount);

  enum CHAR_FLAGS{
      FLAGS_NONE = 0,
      FLAGS_IMMUNE = 1
    };
private:
    

  uint16_t p_height; // Height of Terminal in pixels
  uint16_t p_width; // Width of Terminal in pixels
  uint16_t *p_char_buffer; // Char buffer
  uint8_t p_chars_per_line; // Char per line.
  uint8_t p_lines_per_screen; // Line per screen.
  const lgfx::U8g2font *p_font;
  const lgfx::U8g2font *p_bfont;
  uint8_t p_font_width;
  uint8_t p_font_height;
  uint16_t p_font_color;
  uint16_t p_background_color;
  lgfx::LGFXBase *p_base;
  uint16_t p_cursor;
  uint8_t p_screen_offset;
  const uint8_t p_max_lines = 255;


  void setFontColor(uint16_t color) { p_font_color = color; }
  void setBackgroundColor(uint16_t color) { p_background_color = color; }
  void setHeight(uint16_t height) { p_height = height; }
  void setWidth(uint16_t width) { p_width = width; }
  void setFont(const lgfx::U8g2font *font, const lgfx::U8g2font *bfont) {
    p_font = font;
    p_base->setFont(font);

    if (p_font) {
      p_font_height = p_base->fontHeight();
      p_font_width = p_base->textWidth("M");
      p_chars_per_line = (p_font_width > 0) ? (p_width / p_font_width) : 0;
      p_lines_per_screen = (p_font_height > 0) ? (p_height / p_font_height) : 0;
    }
    p_bfont = bfont;
  }
  void setSprite(lgfx::LGFXBase *sprite) { p_base = sprite; }

  uint8_t readCharFlags(uint32_t location);
  void writeCharFlags(uint32_t location, uint8_t value);

  void discardOldestLine();

};