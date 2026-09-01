#include "global.h"
#include "vecteur.h"
#ifndef NO_LCD
#include "ST7305_U8g2.h" // your provided wrapper
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>


#undef sq
#undef abs
#undef round
#undef min
#undef max
#undef radians
#undef degrees
#undef bit
#undef _round
#undef _abs
#undef _NOP
#undef _min
#undef _max

#include <giac/giac.h>

#include "giac_parser.h"
#include "mathio_ast_printer.h"
#include "mathio_node.h"
#include "mathio_renderer.h"
#include "libbf.h"


class Terminal2 {
public:
  Terminal2() : cursorIndex(0), topLine(0) {}

  void begin() {
    u8g2->setFont(u8g2_font_6x10_tf);
    u8g2->setFontMode(1);  // transparent
    u8g2->setDrawColor(1); // black on white
    lines.push_back("");   // empty input line
  }

  void print(const String &text) {
    // 1. Calculate how many 6-pixel characters fit onto one screen line
    int maxCharsPerLine = (LCD_WIDTH - TERM_LEFT_MARGIN) / 6;
    if (maxCharsPerLine <= 0) maxCharsPerLine = 20; // Fallback safety catch

    if (text.length() == 0) {
      lines.insert(lines.end() - 1, "");
    } else {
      // 2. Chop long output strings (like 2,000 digits) into readable screen rows
      for (unsigned int i = 0; i < text.length(); i += maxCharsPerLine) {
        String chunk = text.substring(i, i + maxCharsPerLine);
        lines.insert(lines.end() - 1, chunk);
      }
    }

    const size_t MAX_SCROLLBACK_LINES = 20; // Total lines allowed in terminal memory
    
    while (lines.size() > MAX_SCROLLBACK_LINES) {
      lines.erase(lines.begin()); 
    }
    
    std::vector<String>(lines).swap(lines);

    scrollToBottom();
    render();
  }

  // Update the current input line
  void setInput(const String &newInput) {
    lines.back() = newInput;
    cursorIndex = newInput.length();
    scrollToBottom();
    render();
  }

  int getCursorIndex() const { return cursorIndex; }

  void setCursor(int idx) {
    if (idx < 0) idx = 0;
    if (idx > (int)lines.back().length()) idx = lines.back().length();
    cursorIndex = idx;
    render();
  }

  void moveCursorLeft() { setCursor(cursorIndex - 1); }
  void moveCursorRight() { setCursor(cursorIndex + 1); }

  void backspace() {
    String &inp = lines.back();
    if (cursorIndex > 0) {
      inp.remove(cursorIndex - 1, 1);
      cursorIndex--;
      render();
    }
  }

  void enter() {
    lines.push_back("");
    cursorIndex = 0;
    topLine = max(0, (int)lines.size() - VISIBLE_ROWS);
    render();
  }

  String getCurrentInput() const { return lines.back(); }

  void scrollToBottom() {
    int total = lines.size();
    if (total > VISIBLE_ROWS)
      topLine = total - VISIBLE_ROWS;
    else
      topLine = 0;
  }

  void render() {
    u8g2->setDrawColor(0); // white
    u8g2->drawBox(0, 0, LCD_WIDTH, LCD_HEIGHT / 2);
    u8g2->setDrawColor(1); // black
    
    int y = TERM_TOP_MARGIN;
    int inputLineIdx = lines.size() - 1;

    for (int i = 0; i < VISIBLE_ROWS; ++i) {
      int lineIdx = topLine + i;
      if (lineIdx >= (int)lines.size())
        break;
      const String &line = lines[lineIdx];

      u8g2->drawStr(TERM_LEFT_MARGIN, y, line.c_str());

      if (lineIdx == inputLineIdx) {
        int cx = u8g2->getStrWidth(line.substring(0, cursorIndex).c_str());
        int cursorX = TERM_LEFT_MARGIN + cx;
        u8g2->drawVLine(cursorX, y - 8, 10);
      }
      y += LINE_HEIGHT;
    }

    u8g2->drawHLine(0, TERM_HEIGHT, LCD_WIDTH);
    u8g2->sendBuffer();
  }

  void renderAll() {
    u8g2->clearBuffer();
    render(); 

    if (mathNeedsRender && currentMathAST) {
      u8g2->setDrawColor(0); // white
      u8g2->drawBox(0, LCD_HEIGHT / 2, LCD_WIDTH, LCD_HEIGHT / 2);
      u8g2->setDrawColor(1); // black
      mathRenderer->render(currentMathAST);
      mathNeedsRender = false; 
    }

    u8g2->sendBuffer();
  }

private:
  std::vector<String> lines;
  int cursorIndex;
  int topLine;
};


EXT_RAM_BSS_ATTR Node pool[MAX_NODE_POOL];

// ---------- Display ----------
#define LCD_WIDTH 400
#define LCD_HEIGHT 300

#define RLCD_SCK_PIN 11
#define RLCD_MOSI_PIN 12
#define RLCD_DC_PIN 5
#define RLCD_CS_PIN 40
#define RLCD_RST_PIN 41

static ST7305_U8g2 lcd(RLCD_SCK_PIN, RLCD_MOSI_PIN, RLCD_DC_PIN, RLCD_CS_PIN,
                       RLCD_RST_PIN);
static U8G2 *u8g2 = nullptr;

EXT_RAM_BSS_ATTR static MathIO::MathRenderer *mathRenderer = nullptr;
Node *currentMathAST = nullptr;      // holds the parsed AST for rendering
static bool mathNeedsRender = false; // flag to trigger redraw
giac::context * ct;

bf_context_t * esp32_bf_context;
// ---------- Terminal geometry (top half) ----------
#define TERM_TOP_MARGIN 5
#define TERM_BOTTOM_MARGIN 5
#define TERM_LEFT_MARGIN 10
#define TERM_RIGHT_MARGIN 10



// The terminal occupies the top half of the screen (0..150)
#define TERM_HEIGHT (LCD_HEIGHT / 2) // 150
#define TERM_USABLE_HEIGHT                                                     \
  (TERM_HEIGHT - TERM_TOP_MARGIN - TERM_BOTTOM_MARGIN)  // 140
#define LINE_HEIGHT 10                                  // font 6x10
#define VISIBLE_ROWS (TERM_USABLE_HEIGHT / LINE_HEIGHT) // 14

// ---------- Forward declarations ----------
void evaluateGiac(const String &input, giac::context *ct);
char keycodeToAscii(uint8_t kc,
                    bool shift); // not used but kept for completeness




Terminal2 term;

// ---------- Giac evaluation (called on Enter) ----------
void evaluateGiac(const String &input, giac::context *ct) {

  static int prompt_counter = 1;

  // term.print(input);  // echo the command

  try {
    giac::gen g(input.c_str(), ct);
    giac::gen res = giac::eval(g, 5, ct);
    String resStr = res.print(ct).c_str();
    term.print(String(prompt_counter++) + "<< " + resStr);

    if (currentMathAST) {
      NodePool::instance().free(currentMathAST);
      currentMathAST = nullptr;
    }
    currentMathAST = MathIO::parseGIAC(input.c_str());

    if (currentMathAST) {
      Serial.println("=== AST structure ===");
      MathIO::printAST(currentMathAST);
      mathNeedsRender = true;
    }
//
    //if (currentMathAST) {
    //  mathNeedsRender = true;
    //} else {
    //  term.print("Warning: Could not parse expression for 2D rendering.");
    //}

    term.renderAll();

    // history management
    giac::vecteur &hin = giac::history_in(ct);
    giac::vecteur &hout = giac::history_out(ct);
    hin.push_back(g);
    hout.push_back(res);

    while (hin.size() > 5) {
      hin.erase(hin.begin());
      giac::vecteur(hin).swap(hin); 
      Serial.println("overflow history_in");
    }
    while (hout.size() > 5) {
      hout.erase(hout.begin());
      giac::vecteur(hout).swap(hout); 
      Serial.println("overflow history_out");
    }
    giac::vecteur &hplot = giac::history_plot(ct);
    if (hplot.size() > 5) {
      hplot.erase(hplot.begin()); // Remove oldest plot object
      giac::vecteur(hplot).swap(hplot); 
    }
  } catch (const std::runtime_error &err) {
    term.print("ERROR: " + String(err.what()));
  } catch (...) {
    term.print("ERROR: Unknown evaluation error");
  }

  // Optional: show heap usage as a normal line
  term.print("Free Heap: " + String(ESP.getFreeHeap()) +
             " bytes, PSRAM: " + String(ESP.getFreePsram()));
  term.print("Waiting for new command");
}



void* esp32_psram_bf_realloc(void *opaque, void *ptr, size_t size) {
    // 1. Free memory if size requested is 0
    if (size == 0) {
        if (ptr != NULL) {
            free(ptr);
        }
        return NULL;
    }

    // 2. Map allocation using the 8-bit capable PSRAM flag
    // (MALLOC_CAP_SPIRAM forces allocation exclusively out of external RAM)
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
}

void giacTask(void *pvParameters) {
  // State machine for escape sequences (arrow keys)
  static bool escSeen = false;
  static bool bracketSeen = false;

  mathRenderer = new MathIO::MathRenderer(u8g2);
  mathRenderer->setArea(0, LCD_HEIGHT / 2, LCD_WIDTH, LCD_HEIGHT / 2);
  ct = new giac::context;
  esp32_bf_context = new bf_context_t;
  bf_context_init(esp32_bf_context, esp32_psram_bf_realloc, NULL);
  bf_ctx_ptr = esp32_bf_context;
  Serial.printf("Allocated %zu bytes in heap for giac context", sizeof(giac::context));

  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();

      //Serial.print("Received: 0x");
      //Serial.println((uint8_t)c, HEX);

      // -------- Parse escape sequences --------

      // 1. Check if we already have ESC [ and are waiting for the command
      // letter
      if (bracketSeen) {
        if (c == 'D') {
          term.moveCursorLeft();
          Serial.println("Left arrow");
        } else if (c == 'C') {
          term.moveCursorRight();
          Serial.println("Right arrow");
        }
        // Reset state
        escSeen = false;
        bracketSeen = false;
        continue;
      }

      // 2. Check if we have just seen ESC
      if (escSeen) {
        if (c == '[') {
          bracketSeen = true; // now we'll wait for the command letter
        } else {
          // Unexpected: reset
          escSeen = false;
        }
        continue;
      }

      // 3. Look for a new ESC
      if (c == 0x1B) {
        escSeen = true;
        continue;
      }

      // -------- Normal character processing --------
      if (c == '\r' || c == '\n') {
        // Enter key
        String input = term.getCurrentInput();
        term.enter();
        evaluateGiac(input, ct);
        //if (bf_ctx_ptr != NULL) {
        //    bf_context_end((bf_context_t *)esp32_bf_context); 
        //    bf_context_init((bf_context_t *)esp32_bf_context, esp32_psram_bf_realloc, NULL);
        //}
        Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes, PSRAM: " + String(ESP.getFreePsram()));
      } else if (c == 0x7F || c == 0x08) {
        // Backspace
        term.backspace();
      } else if (c >= 0x20 && c <= 0x7E) {
        // Printable character
        String input = term.getCurrentInput();
        int pos = term.getCursorIndex();
        input = input.substring(0, pos) + String(c) + input.substring(pos);
        term.setInput(input);
        term.setCursor(pos + 1);
      }
      // Ignore other control characters
    }

    delay(10);
  }
  delete ct;
  delete mathRenderer;
  bf_context_end((bf_context_t *)esp32_bf_context); 
  delete esp32_bf_context;
}

// ---------- Main Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize display
  lcd.begin(0, U8G2_R1);
  u8g2 = lcd.getU8g2();
  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->setFontMode(1);
  u8g2->setDrawColor(1);
  u8g2->enableUTF8Print();

  term.begin();
  term.print("=== ESP32 Giac CAS Terminal ===");
  term.print("Use Serial Monitor to enter expressions.");
  term.print("Type and press Enter to evaluate.");
  term.print("-----------------------------------");
  xTaskCreatePinnedToCore(giacTask, "GiacTask",
                          65536, // 64 KB Stack
                          NULL,
                          1, // Priority
                          NULL,
                          1 // Core 1
  );
}

// ---------- Main Loop: read from Serial ----------
void loop() { vTaskDelete(NULL); }
#else
#include <Arduino.h>

#undef sq
#undef abs
#undef round
#undef min
#undef max
#undef radians
#undef degrees
#undef bit
#undef _round
#undef _abs
#undef _NOP
#undef _min
#undef _max

#include <giac/giac.h>

const size_t MAX_HISTORY = 5;

void giacTask(void *pvParameters) {
  giac::context ct;
  int prompt_counter = 1;

  Serial.printf("\n%d>> ", prompt_counter);

  while (true) {
    if (Serial.available() > 0) {
      String line = Serial.readStringUntil('\n');
      line.trim();

      if (line.length() > 0) {
        Serial.println(line);

        try {
          giac::gen g(line.c_str(), &ct);
          giac::gen res = giac::eval(g, 5, &ct);
          Serial.printf("%d<< %s\n", prompt_counter++, res.print(&ct).c_str());

          giac::vecteur &hin = giac::history_in(&ct);
          giac::vecteur &hout = giac::history_out(&ct);
          hin.push_back(g);
          hout.push_back(res);

          while (hin.size() > MAX_HISTORY) {
            hin.erase(hin.begin());
            Serial.println("overflow history_in");
          }
          while (hout.size() > MAX_HISTORY) {
            hout.erase(hout.begin());
            Serial.println("overflow history_out");
          }
          giac::vecteur &hplot = giac::history_plot(&ct);
          if (hplot.size() > MAX_HISTORY) {
            hplot.erase(hplot.begin()); // Remove oldest plot object
          }

        } catch (const std::runtime_error &err) {
          Serial.printf("ERROR: %s\n", err.what());
        } catch (...) {
          Serial.println("ERROR: Unknown evaluation error");
        }
        Serial.printf("\nFree Heap: %u bytes\n", ESP.getFreeHeap());
        Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
        Serial.printf("Waiting for new Command\n");
        Serial.printf("\n%d>> ", prompt_counter);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000)
    ;
  delay(1000);

  Serial.println("\n=== ESP32 Giac CAS Engine Starting ===");
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());

  // Create a task with 64 KB stack pinned to Core 1
  xTaskCreatePinnedToCore(giacTask, "GiacTask",
                          65536, // 64 KB Stack
                          NULL,
                          1, // Priority
                          NULL,
                          1 // Core 1
  );
}

void loop() {
  vTaskDelete(NULL); // Delete loopTask to free up memory
}
#endif