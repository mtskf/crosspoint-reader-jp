#pragma once
// Host (non-Arduino) stub so PROGMEM font headers compile under gtest.
//
// pgm_read_word / pgm_read_dword use memcpy + an alignment assert so they model the
// ESP32-C3 RISC-V contract: unaligned multi-byte loads fault on device. The hosts
// (x86_64 / arm64) tolerate unaligned access silently, which would let a future
// generator emit a packed struct or odd-offset 16-bit read that passes host tests
// and crashes on hardware. Aborting in the host shim catches that regression.
#ifndef PROGMEM
#define PROGMEM
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
inline uint8_t pgm_read_byte(const void* p) { return *static_cast<const uint8_t*>(p); }
inline uint16_t pgm_read_word(const void* p) {
  assert(reinterpret_cast<uintptr_t>(p) % alignof(uint16_t) == 0 &&
         "pgm_read_word: unaligned read would fault on ESP32-C3");
  uint16_t v;
  std::memcpy(&v, p, sizeof(v));
  return v;
}
inline uint32_t pgm_read_dword(const void* p) {
  assert(reinterpret_cast<uintptr_t>(p) % alignof(uint32_t) == 0 &&
         "pgm_read_dword: unaligned read would fault on ESP32-C3");
  uint32_t v;
  std::memcpy(&v, p, sizeof(v));
  return v;
}
