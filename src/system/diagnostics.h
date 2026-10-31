#pragma once
namespace weby {
// Prints chip, flash, PSRAM and heap facts. Run this on YOUR board and use the
// output to confirm the memory configuration (never assume N8/N16, R2/R8).
void printBootReport();
}
