#include "terminal.hpp"
#include "global.hpp"
#include <cstdint>
#include <iostream>
#include <stdint.h>

Terminal::Terminal() {
  // TODO: free raw memory, close files, etc.
  // Example: delete[] m_buffer;
}

Terminal::~Terminal() { deinit(); }

uint8_t Terminal::readCharFlags(uint32_t location) {
  if (!p_char_buffer || location >= p_chars_per_line * p_max_lines)
    return 0;
  return p_char_buffer[location] >> 8;
}

void Terminal::writeCharFlags(uint32_t location, uint8_t value) {
  if (!p_char_buffer || location >= p_chars_per_line * p_max_lines)
    return;
  p_char_buffer[location] &= 0x00FF;
  p_char_buffer[location] |= (uint16_t)value << 8;
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

  p_char_buffer = (uint16_t *)calcemistMalloc(
      sizeof(uint16_t) * (p_chars_per_line * p_max_lines + 1));
  if (p_char_buffer)
    memset(p_char_buffer, 0,
           sizeof(uint16_t) * (p_chars_per_line * p_max_lines + 1));
  p_cursor = 0;
  p_screen_offset = 0;
}

void Terminal::deinit() {
  if (p_char_buffer)
    calcemistFree(p_char_buffer);
}

void Terminal::discardOldestLine() {
  size_t line_size = p_chars_per_line;
  size_t total_chars = p_chars_per_line * p_max_lines;
  memmove(p_char_buffer, p_char_buffer + line_size,
          sizeof(uint16_t) * line_size * (p_max_lines - 1));
  memset(p_char_buffer + line_size * (p_max_lines - 1), 0,
         sizeof(uint16_t) * (line_size));
  //std::cout << "discard old line";
}

void Terminal::addChar(char c) { addChar(c, FLAGS_NONE); }

void Terminal::addChar(char c, uint8_t flags) {
  // try to find the last valid char in the input

  uint32_t end = p_cursor;
  while (end < p_chars_per_line * p_max_lines &&
         (p_char_buffer[end] & 0x00FF) != 0) {
    end++;
  }
  if (end + 1 < p_chars_per_line * p_max_lines) {
    for (uint32_t i = end; i > p_cursor; i--) {
      p_char_buffer[i] = p_char_buffer[i - 1];
    }
  }
  p_char_buffer[p_cursor] = ((uint16_t)flags << 8) | c;
  p_cursor++;
  if (p_cursor >= p_chars_per_line * p_max_lines) {
    discardOldestLine();
    p_cursor = p_chars_per_line * (p_max_lines - 1);
  }
  while (p_cursor >=
         p_chars_per_line * (p_lines_per_screen + p_screen_offset)) {
    p_screen_offset++;
  }
  // std::cout << p_screen_offset << " " << c << " " << flags << std::endl;
}

int Terminal::getCursorIndex() {
  // TODO: implement
  return rand() * p_max_lines;
}

void Terminal::scroll(int8_t amount) {
  if (amount < 0 && p_screen_offset >= -amount)
    p_screen_offset += amount;
  if (amount > 0 && p_screen_offset <= p_max_lines - amount &&
      p_cursor >= p_chars_per_line * (p_lines_per_screen + p_screen_offset))
    p_screen_offset += amount;
}

void Terminal::setCursor(int idx) {
  // TODO: implement
}

void Terminal::moveCursorLeft() {
  if (p_cursor == 0 || readCharFlags(p_cursor) & FLAGS_IMMUNE)
    return;
  p_cursor--;
}

void Terminal::moveCursorRight() {
  if (p_cursor + 1 > (p_chars_per_line * p_max_lines) ||
      readCharFlags(p_cursor) & FLAGS_IMMUNE)
    return;
  p_cursor++;
}

void Terminal::backspace() {
  if (p_cursor == 0)
    return;
  if (readCharFlags(p_cursor - 1) & FLAGS_IMMUNE)
    return;

  uint32_t end = p_cursor;
  while (end < p_chars_per_line * p_max_lines &&
         (p_char_buffer[end] & 0x00FF) != 0) {
    end++;
  }
  p_cursor--;
  for (uint32_t i = p_cursor; i < end; i++) {
    p_char_buffer[i] = p_char_buffer[i + 1];
  }
  p_char_buffer[end] = 0;
}

void Terminal::enter() {
  while (p_cursor < (p_chars_per_line * p_max_lines) &&
         p_char_buffer[p_cursor] != 0)
    p_cursor++;
  uint32_t start_loc = p_cursor;
  for (uint32_t i = start_loc;
       i < ((start_loc / p_chars_per_line) + 1) * p_chars_per_line; i++) {
    p_char_buffer[i] = 0;
    p_cursor = i + 1;
  }

  if (p_cursor >= p_chars_per_line * p_max_lines) {
    discardOldestLine();
    p_cursor = p_chars_per_line * (p_max_lines - 1);
  }
  while (p_cursor >=
         p_chars_per_line * (p_lines_per_screen + p_screen_offset)) {
    p_screen_offset++;
  }
}

char *Terminal::getCurrentInput() {
  uint32_t location = p_cursor;
  while (location != 0) {
    if (readCharFlags(location - 1) & FLAGS_IMMUNE) {
      break;
    }
    location--;
  }
  uint32_t start = location;
  for (uint32_t i = start; i < p_chars_per_line * p_max_lines; i++) {
    if (p_char_buffer[i] == 0) {
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
    buffer[i] = p_char_buffer[start + i] & 0x00FF;
  }
  buffer[len] = '\0';
  std::cout << buffer << " with size: " << len << std::endl;
  return buffer;
}

void Terminal::render(uint16_t x, uint16_t y) {

  p_base->setTextColor(p_font_color);
  p_base->fillRect(x, y, p_width, p_height, p_background_color);
  uint16_t x_offset = (p_width - (p_font_width * p_chars_per_line)) / 2;
  uint16_t y_offset = (p_height - (p_font_height * p_lines_per_screen)) / 2;
  for (uint32_t i = p_screen_offset * p_chars_per_line;
       i < (p_chars_per_line * p_lines_per_screen) +
               p_screen_offset * p_chars_per_line;
       i++) {
    bool immune = readCharFlags(i) & FLAGS_IMMUNE;
    uint32_t j = i - p_screen_offset * p_chars_per_line;
    if ((p_char_buffer[i] & 0x00FF) != 0) {
      if (immune)
        p_base->setFont(p_bfont);
      else
        p_base->setFont(p_font);
      p_base->drawChar((p_char_buffer[i] & 0x00FF),
                       (j % p_chars_per_line) * p_font_width + x_offset,
                       (j / p_chars_per_line) * p_font_height + p_font_height +
                           y_offset);
    } else if (immune) {
      p_base->setFont(p_bfont);
      p_base->drawChar('$', (j % p_chars_per_line) * p_font_width + x_offset,
                       (j / p_chars_per_line) * p_font_height + p_font_height +
                           y_offset);
    }
  }

  bool state =
      (millis() % 1000) < 200 &&
      !(p_cursor >= p_chars_per_line * (p_lines_per_screen + p_screen_offset) ||
        p_cursor < p_chars_per_line * (p_screen_offset));
  p_base->drawLine(
      (p_cursor % p_chars_per_line) * p_font_width + x_offset - 1,
      ((p_cursor / p_chars_per_line) - p_screen_offset) * p_font_height +
          y_offset + 1,
      (p_cursor % p_chars_per_line) * p_font_width + x_offset - 1,
      ((p_cursor / p_chars_per_line) - p_screen_offset) * p_font_height +
          y_offset + p_font_height - 1,
      state ? p_font_color : p_background_color);

  // TODO: implement
}
