#include "lexer.h"
class Terminal {
public:
  Terminal();
  ~Terminal();

  void begin();

  void addChar(char);
  int getCursorIndex();
  void setCursor(int idx);
  void moveCursorLeft();
  void moveCursorRight();
  void backspace();
  void enter();
  char * getCurrentInput();
  void scrollToBottom();
  void render();
  void renderAll();
private:
    uint16_t height;
    uint16_t width;
    uint8_t current_history = 0;
    const int num_history = 10;
    char ** history;
    char * screen;
    uint8_t chars_x;
    uint8_t chars_y;
    
};