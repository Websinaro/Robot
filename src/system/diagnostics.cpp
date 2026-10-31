#include "system/diagnostics.h"
#include <Arduino.h>
#include "system/log.h"
#include "version.h"

namespace weby {

void printBootReport() {
  Serial.println();
  Serial.println("=== Weby boot report ===");
  Serial.printf("firmware      : %s\n", WEBY_FW_VERSION);
  Serial.printf("chip          : %s rev %d, %d core(s) @ %lu MHz\n", ESP.getChipModel(),
                (int)ESP.getChipRevision(), (int)ESP.getChipCores(),
                (unsigned long)ESP.getCpuFreqMHz());
  Serial.printf("sdk           : %s\n", ESP.getSdkVersion());
  Serial.printf("flash size    : %lu bytes (as seen by firmware)\n", (unsigned long)ESP.getFlashChipSize());
  Serial.printf("flash speed   : %lu Hz\n", (unsigned long)ESP.getFlashChipSpeed());
  Serial.printf("PSRAM found   : %s\n", psramFound() ? "yes" : "no");
  Serial.printf("PSRAM size    : %lu bytes\n", (unsigned long)ESP.getPsramSize());
  Serial.printf("heap total    : %lu bytes\n", (unsigned long)ESP.getHeapSize());
  Serial.printf("heap free     : %lu bytes\n", (unsigned long)ESP.getFreeHeap());
  Serial.printf("heap min free : %lu bytes\n", (unsigned long)ESP.getMinFreeHeap());
  Serial.printf("max alloc blk : %lu bytes\n", (unsigned long)ESP.getMaxAllocHeap());
  Serial.println("========================");
}

}  // namespace weby
