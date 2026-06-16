#pragma once
// Host (non-Arduino) stub so PROGMEM font headers compile under gtest.
#ifndef PROGMEM
#define PROGMEM
#endif
#include <cstdint>
inline uint8_t pgm_read_byte(const void* p) { return *static_cast<const uint8_t*>(p); }
inline uint16_t pgm_read_word(const void* p) { return *static_cast<const uint16_t*>(p); }
