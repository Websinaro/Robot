#include "server/response_validator.h"
#include <string.h>

namespace weby {
namespace {

bool terminated(const char* s, size_t cap) { return memchr(s, '\0', cap) != 0; }

bool idChars(const char* s) {
  for (; *s; ++s) {
    char c = *s;
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    if (!ok) return false;
  }
  return true;
}

bool oneOf(const char* s, const char* const* list, size_t n) {
  for (size_t i = 0; i < n; ++i) if (strcmp(s, list[i]) == 0) return true;
  return false;
}

const char* const kTypes[] = {"chat", "search", "game", "system"};

}  // namespace

ErrorCode validateResponse(const ConversationResponse& r) {
  if (!terminated(r.requestId, sizeof r.requestId) || !terminated(r.status, sizeof r.status) ||
      !terminated(r.transcript, sizeof r.transcript) || !terminated(r.replyText, sizeof r.replyText) ||
      !terminated(r.emotion, sizeof r.emotion) || !terminated(r.interactionType, sizeof r.interactionType) ||
      !terminated(r.audio.format, sizeof r.audio.format))
    return ErrorCode::PAYLOAD_INVALID;

  if (r.requestId[0] == 0 || !idChars(r.requestId)) return ErrorCode::PAYLOAD_INVALID;
  if (strcmp(r.status, "ok") != 0) return ErrorCode::PAYLOAD_INVALID;
  if (!oneOf(r.interactionType, kTypes, sizeof kTypes / sizeof kTypes[0])) return ErrorCode::PAYLOAD_INVALID;
  if (r.displayDurationMs > kMaxDisplayMs) return ErrorCode::PAYLOAD_INVALID;
  if (r.replyText[0] == 0 && !r.audio.present) return ErrorCode::PAYLOAD_INVALID;  // nothing to show or say

  if (r.audio.present) {
    if (strcmp(r.audio.format, "pcm_s16le") != 0) return ErrorCode::PAYLOAD_INVALID;
    if (r.audio.channels != 1) return ErrorCode::PAYLOAD_INVALID;
    if (r.audio.sampleRate != 16000 && r.audio.sampleRate != 22050 && r.audio.sampleRate != 24000)
      return ErrorCode::PAYLOAD_INVALID;
    if (r.audio.sizeBytes == 0) return ErrorCode::PAYLOAD_INVALID;
    if (r.audio.sizeBytes > kMaxAudioBytes) return ErrorCode::PAYLOAD_TOO_LARGE;
  }
  return ErrorCode::OK;
}

Emotion resolveEmotion(const ConversationResponse& r) {
  if (!terminated(r.emotion, sizeof r.emotion)) return Emotion::IDLE;
  return emotionFromServer(r.emotion);
}

}  // namespace weby
