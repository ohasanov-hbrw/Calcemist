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