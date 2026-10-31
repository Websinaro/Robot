#pragma once
#include <stdint.h>

namespace weby {

// Ranges: 0x00xx general, 0x01xx hardware, 0x02xx audio, 0x03xx network,
//         0x04xx storage, 0x05xx protocol/security.
enum class ErrorCode : uint16_t {
  OK = 0x0000,
  INVALID_ARGUMENT = 0x0001,
  OUT_OF_MEMORY = 0x0002,
  TIMEOUT = 0x0003,

  DISPLAY_INIT_FAILED = 0x0100,
  SD_NOT_PRESENT = 0x0101,
  SD_MOUNT_FAILED = 0x0102,

  I2S_INIT_FAILED = 0x0200,
  AUDIO_UNDERRUN = 0x0201,
  AUDIO_OVERFLOW = 0x0202,

  WIFI_CONNECT_FAILED = 0x0300,
  SERVER_UNREACHABLE = 0x0301,
  TLS_VERIFY_FAILED = 0x0302,

  NVS_FAILED = 0x0400,
  FILE_NOT_FOUND = 0x0401,
  FILE_CORRUPT = 0x0402,

  AUTH_REJECTED = 0x0500,
  AUTH_EXPIRED = 0x0501,
  PAYLOAD_TOO_LARGE = 0x0502,
  PAYLOAD_INVALID = 0x0503,
};

inline bool ok(ErrorCode e) { return e == ErrorCode::OK; }

}  // namespace weby
