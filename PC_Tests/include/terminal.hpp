#pragma once
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
  void addChar(char);
  void addCharTerm(char);
  int getCursorIndex();
  void setCursor(int idx);
  void moveCursorLeft();
  void moveCursorRight();
  void backspace();
  void enter();
  char *getCurrentInput();
  void render(uint16_t x, uint16_t y);

private:
  uint16_t p_height;
  uint16_t p_width;
  uint8_t p_current_history = 0;
  const int num_history = 10;
  char **p_history;
  char *p_screen;
  uint8_t *p_char_type;
  uint8_t p_chars_x;
  uint8_t p_chars_y;
  const lgfx::U8g2font *p_font;
  const lgfx::U8g2font *p_bfont;
  uint8_t p_font_width;
  uint8_t p_font_height;
  uint16_t p_font_color;
  uint16_t p_background_color;
  lgfx::LGFXBase *p_base;
  uint16_t p_cursor;
  bool p_char_changed;

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
      p_chars_x = (p_font_width > 0) ? (p_width / p_font_width) : 0;
      p_chars_y = (p_font_height > 0) ? (p_height / p_font_height) : 0;
    }
    p_bfont = bfont;
  }
  void setSprite(lgfx::LGFXBase *sprite) { p_base = sprite; }
  void initCharType(uint32_t numchars);
  bool readCharType(uint32_t location);
  void writeCharType(uint32_t location, bool value);
  void scrollOneLine();

};