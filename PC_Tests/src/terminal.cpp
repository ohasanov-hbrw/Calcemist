#include "terminal.hpp"
#include "global.hpp"
#include <stdint.h>
#include <iostream>

Terminal::Terminal() {
  // TODO: free raw memory, close files, etc.
  // Example: delete[] m_buffer;
}

Terminal::~Terminal() { deinit(); }

void Terminal::initCharType(uint32_t numchars) {
  p_char_type =
      (uint8_t *)calcemistMalloc(((numchars) / 8 + 1) * sizeof(uint8_t));
  if (!p_char_type) {
    // Handle allocation failure gracefully (e.g., set an error flag)
    return;
  }
  memset(p_char_type, 0, ((numchars) / 8 + 1) * sizeof(uint8_t));
}

bool Terminal::readCharType(uint32_t location) {
  if (!p_char_type || location >= p_chars_x * p_chars_y)
    return false; // Bounds check!
  return (p_char_type[location / 8] & (0x1 << (location % 8))) != 0x00;
}

void Terminal::writeCharType(uint32_t location, bool value) {
  if (!p_char_type || location >= p_chars_x * p_chars_y)
    return; // Bounds check!
  if (value)
    p_char_type[location / 8] |=
        (uint8_t)(0x1 << (location % 8)); // Cast to uint8_t
  else
    p_char_type[location / 8] &= (uint8_t)~(0x1 << (location % 8));
}

void Terminal::init(lgfx::LGFXBase *sprite, uint16_t width, uint16_t height,
                    const lgfx::U8g2font *font, const lgfx::U8g2font *bfont,
                    uint16_t f_color, uint16_t b_color) {
  deinit();
  setSprite(sprite);
  setHeight(height);
  setWidth(width);
  setFontColor(f_color);
  setBackgroundColor(b_color);
  setFont(font, bfont);

  p_screen =
      (char *)calcemistMalloc(sizeof(char) * (p_chars_x * p_chars_y + 1));
  if (p_screen)
    memset(p_screen, 0, sizeof(char) * (p_chars_x * p_chars_y + 1));

  // for(uint32_t i = 0; i < (p_chars_x * p_chars_y); i++){
  //   p_screen[i] = 'a' + rand() % 26;
  // }
  initCharType(p_chars_x * p_chars_y);
  p_cursor = 0;
  // TODO: implement
}

void Terminal::deinit() {
  if (p_screen)
    calcemistFree(p_screen);
  if (p_char_type)
    calcemistFree(p_char_type);
}

void Terminal::scrollOneLine() {
  size_t line_size = p_chars_x;
  size_t total_chars = p_chars_x * p_chars_y;
  memmove(p_screen, p_screen + line_size,
          sizeof(char) * line_size * (p_chars_y - 1));
  memset(p_screen + line_size * (p_chars_y - 1), 0, sizeof(char) * (line_size));

  for (size_t line = 1; line < p_chars_y; ++line) {
    for (size_t col = 0; col < line_size; ++col) {
      uint32_t src_idx = line * line_size + col;
      uint32_t dst_idx = (line - 1) * line_size + col;
      bool type = readCharType(src_idx);
      writeCharType(dst_idx, type);
    }
  }

  // 3. Clear the types for the last (new) line
  for (size_t col = 0; col < line_size; ++col) {
    uint32_t idx = (p_chars_y - 1) * line_size + col;
    writeCharType(idx, false);
  }
}

void Terminal::addChar(char c) {
  if (!c) return;
  uint32_t end = p_cursor;
  while (end < p_chars_x * p_chars_y && p_screen[end] != 0) {
    end++;
  }
  if (end + 1 < p_chars_x * p_chars_y) {
    for (uint32_t i = end; i > p_cursor; i--) {
      p_screen[i] = p_screen[i - 1];
      // Shift the bit as well
      bool bit_val = readCharType(i - 1);
      writeCharType(i, bit_val);
    }
  }
  p_screen[p_cursor] = c;
  writeCharType(p_cursor, false); 
  p_cursor++;
  p_char_changed = true;
  if (p_cursor >= p_chars_x * p_chars_y) {
    scrollOneLine();
    p_cursor = p_chars_x * (p_chars_y - 1);
  }
}

void Terminal::addCharTerm(char c) {
  uint32_t end = p_cursor;
  while (end < p_chars_x * p_chars_y && p_screen[end] != 0) {
    end++;
  }
  if (end + 1 < p_chars_x * p_chars_y) {
    for (uint32_t i = end; i > p_cursor; i--) {
      p_screen[i] = p_screen[i - 1];
      // Shift the bit as well
      bool bit_val = readCharType(i - 1);
      writeCharType(i, bit_val);
    }
  }
  p_screen[p_cursor] = c;
  writeCharType(p_cursor, true); 
  p_cursor++;
  p_char_changed = true;
  if (p_cursor >= p_chars_x * p_chars_y) {
    scrollOneLine();
    p_cursor = p_chars_x * (p_chars_y - 1);
  }
}

int Terminal::getCursorIndex() {
  // TODO: implement
  return 0;
}

void Terminal::setCursor(int idx) {
  // TODO: implement
}

void Terminal::moveCursorLeft() {
  if(p_cursor == 0 || readCharType(p_cursor - 1)) return;
  p_cursor--;
  // TODO: implement
}

void Terminal::moveCursorRight() {
  if(p_cursor + 1 > (p_chars_x * p_chars_y) || p_screen[p_cursor] == 0) return;
  p_cursor++;
  // TODO: implement
}

void Terminal::backspace() {
  if (p_cursor == 0) return;
  if (readCharType(p_cursor - 1))
    return;

  // 1. Find the end of the current string
  uint32_t end = p_cursor;
  while (end < p_chars_x * p_chars_y && p_screen[end] != 0) {
    end++;
  }

  // 2. Move cursor back one
  p_cursor--;

  // 3. Shift all characters to the left (including bitmask)
  for (uint32_t i = p_cursor; i < end; i++) {
    p_screen[i] = p_screen[i + 1];
    // Shift the bit as well
    bool bit_val = readCharType(i + 1);
    writeCharType(i, bit_val);
  }

  // 4. Clear the last character
  p_screen[end] = 0;
  writeCharType(end, false);

  p_char_changed = true;
}

void Terminal::enter() {
  while(p_cursor < (p_chars_x * p_chars_y) && p_screen[p_cursor] != 0) p_cursor++;
  uint32_t start_loc = p_cursor;
  for (uint32_t i = start_loc; i < ((start_loc / p_chars_x) + 1) * p_chars_x;
       i++) {
    p_screen[i] = 0;
    p_cursor = i + 1;
  }

  if (p_cursor >= p_chars_x * p_chars_y) {
    scrollOneLine();
    p_cursor = p_chars_x * (p_chars_y - 1);
  }


  // TODO: implement
}

char *Terminal::getCurrentInput() {
  uint32_t location = p_cursor;
  while (location != 0) {
    if (readCharType(location - 1)) {
      break;
    }
    location--;
  }
  uint32_t start = location;
  for (uint32_t i = start; i < p_chars_x * p_chars_y; i++) {
    if (p_screen[i] == 0) {
      location = i;
      break;
    }
  }
  uint32_t len = location - start;
  char *buffer = (char *)calcemistMalloc(
      (len + 1) * sizeof(char)); // +1 for null terminator
  if (!buffer)
    return nullptr;
  for (uint32_t i = 0; i < len; i++) {
    buffer[i] = p_screen[start + i];
  }
  buffer[len] = '\0';
  std::cout << buffer << "  size: " << len << std::endl;
  return buffer;
}

void Terminal::render(uint16_t x, uint16_t y) {

  p_base->setTextColor(p_font_color);
  p_base->fillRect(x, y, p_width, p_height, p_background_color);
  uint16_t x_offset = (p_width - (p_font_width * p_chars_x)) / 2;
  uint16_t y_offset = (p_height - (p_font_height * p_chars_y)) / 2;
  for (uint32_t i = 0; i < (p_chars_x * p_chars_y); i++) {
    bool term = readCharType(i);
    if (p_screen[i] != 0) {
      if (term)
        p_base->setFont(p_bfont);
      else
        p_base->setFont(p_font);
      p_base->drawChar(p_screen[i], (i % p_chars_x) * p_font_width + x_offset,
                       (i / p_chars_x) * p_font_height + p_font_height +
                           y_offset);
    } else if (term) {
      p_base->setFont(p_bfont);
      p_base->drawChar('$', (i % p_chars_x) * p_font_width + x_offset,
                       (i / p_chars_x) * p_font_height + p_font_height +
                           y_offset);
    }
  }

  bool state = (millis() % 1000) < 200 || p_char_changed;
  p_base->drawLine((p_cursor % p_chars_x) * p_font_width + x_offset - 1,
                   (p_cursor / p_chars_x) * p_font_height + y_offset + 1,
                   (p_cursor % p_chars_x) * p_font_width + x_offset - 1,
                   (p_cursor / p_chars_x) * p_font_height + y_offset +
                       p_font_height - 1,
                   state ? p_font_color : p_background_color);
  p_char_changed = false;

  // TODO: implement
}
