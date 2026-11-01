#pragma once
// Bounded, fixed-size representation of a server conversation response plus strict
// validation. JSON -> struct parsing is a separate step (needs a JSON library); every
// value MUST pass validateResponse() before any module acts on it.
#include <stddef.h>
#include <stdint.h>
#include "error_codes.h"
#include "personality/emotions.h"

namespace weby {

const size_t   kMaxRequestId   = 40;
const size_t   kMaxTranscript  = 256;
const size_t   kMaxReplyText   = 1024;
const size_t   kMaxEmotionName = 24;
const size_t   kMaxAudioBytes  = 2u * 1024u * 1024u;
const uint16_t kMaxDisplayMs   = 60000;

struct AudioMeta {
  bool present;
  char format[16];          // "pcm_s16le" is the only format V1 firmware plays
  uint32_t sampleRate;
  uint8_t channels;
  uint32_t sizeBytes;
  AudioMeta() : present(false), sampleRate(0), channels(0), sizeBytes(0) { format[0] = 0; }
};

struct ConversationResponse {
  char requestId[kMaxRequestId];
  char status[12];                  // "ok"
  char transcript[kMaxTranscript];
  char replyText[kMaxReplyText];
  char emotion[kMaxEmotionName];
  char interactionType[16];         // chat | search | game | system
  uint16_t displayDurationMs;
  AudioMeta audio;
  ConversationResponse() : displayDurationMs(0) {
    requestId[0] = status[0] = transcript[0] = replyText[0] = emotion[0] = interactionType[0] = 0;
  }
};

// OK, PAYLOAD_INVALID or PAYLOAD_TOO_LARGE.
ErrorCode validateResponse(const ConversationResponse& r);
// Unknown / empty emotion -> IDLE (neutral).
Emotion resolveEmotion(const ConversationResponse& r);

}  // namespace weby
