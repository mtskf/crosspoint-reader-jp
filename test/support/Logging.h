#pragma once
// Host (non-Arduino) stub so BidiUtils.cpp and any other lib files that
// include <Logging.h> compile under gtest without pulling in HardwareSerial.h.
// All log macros are silenced on host — they emit nothing. This matches the
// device-side ENABLE_SERIAL_LOG=off behaviour (release builds).

#define LOG_DBG(origin, format, ...)
#define LOG_ERR(origin, format, ...)
#define LOG_INF(origin, format, ...)
