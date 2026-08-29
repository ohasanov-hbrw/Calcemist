#include "terminal.hpp"
#include "global.hpp"
#include <stdint.h>

Terminal::Terminal() {
  // TODO: free raw memory, close files, etc.
  // Example: delete[] m_buffer;
}

Terminal::~Terminal() { deinit(); }

void Terminal::init(lgfx::LGFXBase *sprite, uint16_t width, uint16_t height,
                    const lgfx::U8g2font *font, uint16_t f_color,
                    uint16_t b_color) {
  deinit();
  setSprite(sprite);
  setHeight(height);
  setWidth(width);
  setFontColor(f_color);
  setBackgroundColor(b_color);
  setFont(font);

  p_screen = (char *)calcemistMalloc(sizeof(char) * p_chars_x * p_chars_y);
  if (p_screen)
    memset(p_screen, 0, sizeof(char) * p_chars_x * p_chars_y);

  // for(uint32_t i = 0; i < (p_chars_x * p_chars_y); i++){
  //   p_screen[i] = 'a' + rand() % 26;
  // }

  p_cursor = 0;
  // TODO: implement
}

void Terminal::deinit() {
  if (p_screen)
    calcemistFree(p_screen);
}

void Terminal::addChar(char c) {
  if (!c)
    return;
  p_screen[p_cursor] = c;
  p_cursor++;
  p_char_changed = true;
  p_cursor %= (p_chars_x * p_chars_y);
}

int Terminal::getCursorIndex() {
  // TODO: implement
  return 0;
}

void Terminal::setCursor(int idx) {
  // TODO: implement
}

void Terminal::moveCursorLeft() {
  // TODO: implement
}

void Terminal::moveCursorRight() {
  // TODO: implement
}

void Terminal::backspace() {
  p_cursor--;
  p_cursor %= (p_chars_x * p_chars_y);
  p_screen[p_cursor] = 0;
  p_char_changed = true;
  // TODO: implement
}

void Terminal::enter() {
    uint32_t start_loc = p_cursor;
    for(uint32_t i = start_loc; i < ((start_loc / p_chars_x) + 1) * p_chars_x; i++){
        p_screen[i] = 0;
        p_cursor = i + 1;
    }
  // TODO: implement
}

char *Terminal::getCurrentInput() {
  // TODO: implement
  return nullptr;
}

void Terminal::scrollToBottom() {
  // TODO: implement
}

void Terminal::render(uint16_t x, uint16_t y) {
  p_base->setFont(p_font);
  p_base->setTextColor(p_font_color);
  p_base->fillRect(x, y, p_width, p_height, p_background_color);
  uint16_t x_offset = (p_width - (p_font_width * p_chars_x)) / 2;
  uint16_t y_offset = (p_height - (p_font_height * p_chars_y)) / 2;
  for (uint32_t i = 0; i < (p_chars_x * p_chars_y); i++) {
    if (p_screen[i] != 0)
      p_base->drawChar(p_screen[i], (i % p_chars_x) * p_font_width + x_offset,
                       (i / p_chars_x) * p_font_height + p_font_height +
                           y_offset);
  }

  bool state = (millis() % 1000) < 200 || p_char_changed;
  p_base->drawLine((p_cursor % p_chars_x) * p_font_width + x_offset,
                   (p_cursor / p_chars_x) * p_font_height + y_offset + 1,
                   (p_cursor % p_chars_x) * p_font_width + x_offset,
                   (p_cursor / p_chars_x) * p_font_height + y_offset +
                       p_font_height - 1,
                   state ? p_font_color : p_background_color);
  p_char_changed = false;

  // TODO: implement
}
