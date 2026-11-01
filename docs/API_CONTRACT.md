# Weby device <-> server API contract, version 1

**Status: DEFINED, NOT IMPLEMENTED.** No server code was provided or inspected. Nothing below is operational until
the Node.js/TypeScript server implements it. Firmware-side parsing of `ConversationResponse` is validated by
`server/response_validator` (host-tested); the HTTP client that calls these endpoints is not written yet.

## Conventions
- Base URL: `https://<host>/api/v1` (HTTPS only, certificate verified, never disabled).
- Auth: `Authorization: Bearer <access_token>` on every endpoint except provisioning. **No tokens in URLs, query
  strings or fragments.**
- Headers on every request: `X-Device-Id`, `X-Request-Id` (unique per request, `[A-Za-z0-9_-]{1,40}`),
  `X-Fw-Version`. The server rejects a repeated `X-Request-Id` from the same device within 5 minutes (replay protection).
- JSON is UTF-8, `Content-Type: application/json`. Unknown response fields are ignored; missing required ones are errors.
- Compatibility: path version `v1`; response field `api_version: 1`. A major bump changes the path.
- Error body (any non-2xx): `{"status":"error","error":{"code":"<CODE>","message":"<text>","retry_after_s":<int|null>}}`
  Codes: `AUTH_REQUIRED`, `AUTH_EXPIRED`, `DEVICE_REVOKED`, `PAYLOAD_TOO_LARGE`, `PAYLOAD_INVALID`, `RATE_LIMITED`,
  `STT_FAILED`, `AI_UNAVAILABLE`, `SEARCH_UNAVAILABLE`, `INTERNAL`.
- Firmware retry policy (`network/retry_policy`): 2xx success; 401 -> re-authenticate; 408/425/429/5xx (except 501/505)
  and transport errors -> retry with exponential backoff (base 500 ms, cap 30 s, 5 attempts, equal jitter); other 4xx -> fail.

## Provisioning (pairing-code flow; no secrets in firmware)
1. `POST /device/register` (no auth)
   Req: `{"device_id":"<chip-derived id>","hw":"esp32-s3","fw":"0.1.0"}`
   Res 200: `{"api_version":1,"pairing_code":"ABCD-1234","expires_in":600,"poll_interval_s":3}`
   The device shows the code on screen; the owner enters it in the server's web UI.
2. `POST /device/verify` (no auth, polled)
   Req: `{"device_id":"...","pairing_code":"ABCD-1234"}`
   Res 202 `{"status":"pending"}` | 200 `{"status":"approved","access_token":"...","expires_in":3600,"refresh_token":"..."}` | 403 `DEVICE_REVOKED`.
   Tokens are stored in NVS (see firmware notes below). Timeout 10 s; body <= 1 KB.
3. `POST /device/token/refresh` (no Bearer; refresh token in the JSON body)
   Req: `{"device_id":"...","refresh_token":"..."}` -> 200 same shape as verify. 401/403 -> back to step 1.
Revocation: the server invalidates tokens; the next request returns 401/`DEVICE_REVOKED`, and the device returns to pairing.

## Voice conversation
`POST /voice/conversation`
- Req headers: `Content-Type: audio/L16;rate=16000;channels=1` (raw PCM s16le), `X-Session-Id` (optional).
- Req body: audio, max **480,000 bytes** (15 s). Timeout: 20 s upload + 30 s wait for the reply.
- Res 200 JSON (`ConversationResponse`):
```json
{
  "api_version": 1,
  "status": "ok",
  "request_id": "req_8f2c",
  "session_id": "sess_19ab",
  "interaction_type": "chat",
  "transcript": "what is the moon",
  "reply_text": "The Moon is Earth's natural satellite.",
  "emotion": "curious",
  "display_content": {"type": "card", "title": "The Moon", "body": "Earth's only natural satellite.", "duration_ms": 8000, "page": 1, "pages": 1},
  "speech_audio": {"audio_id": "aud_77d1", "format": "pcm_s16le", "sample_rate": 24000, "channels": 1, "size_bytes": 96000},
  "memory_updates": [{"key": "pref.topic", "value": "space", "version": 12}]
}
```
- Empty speech: `{"status":"ok", "interaction_type":"system", "transcript":"", "reply_text":"I didn't catch that.", ...}` (never a hang).
- Limits enforced by firmware: `reply_text` <= 1023 chars, `transcript` <= 255, `request_id` <= 39 chars,
  `emotion` <= 23 chars (unknown emotion -> neutral), audio <= 2 MiB, `display duration` <= 60 s, response JSON <= 8 KiB.

`GET /voice/audio/{audio_id}` -> `200`, `Content-Type: audio/L16;rate=24000;channels=1`, chunked stream of raw PCM.
Auth header required; `audio_id` must be unguessable and bound to the device. Timeout: first byte 10 s, stalls > 5 s abort.
Only `pcm_s16le`, mono, 16000/22050/24000 Hz are accepted in V1 (no MP3/Opus decoder on the device).
**WebSocket streaming is NOT part of V1**; it may be added in a later API version if latency requires it.

## Text conversation
`POST /chat` Req `{"text":"...","session_id":"..."}` (text <= 512 chars). Res: same `ConversationResponse`. Timeout 30 s; body <= 2 KB.

## Search
`POST /search` Req `{"query":"...","session_id":"...","page":1}` (query <= 256 chars).
Res: `ConversationResponse` with `interaction_type:"search"` and `display_content`
`{"type":"search_result","title":"...","summary":"...","source":"example.org","page":1,"pages":3}`.
The server returns clean bounded text; the firmware never fetches third-party pages. Timeout 30 s.

## Memory synchronisation
`POST /memory/sync`
Req: `{"since_version":12,"changes":[{"key":"pref.volume","value":"60"}]}` (<= 8 changes; key `[a-z0-9_.]{1,23}`; value <= 63 chars, no control chars).
Res: `{"status":"ok","server_version":15,"applied":["pref.volume"],"updates":[{"key":"pref.color","value":"blue","version":14}]}`.
An empty value deletes a key. The device applies an update only if its version is newer than the stored one
(replays are ignored), and removes a pending change only if it was not edited during the request. Timeout 15 s; <= 4 KB.

## Device status / config
`POST /device/status` Req `{"fw":"0.1.0","state":"ONLINE_IDLE","uptime_s":3600,"heap_min":120000,"rssi":-58,"last_error":"0x0301"}` -> 204. No tokens, passwords or user speech. Timeout 10 s.
`GET /device/config` -> `{"api_version":1,"min_fw":"0.1.0","wake_word":{"phrase":"Hey Weby","model_version":null},"volume_default":60}`. Timeout 10 s.
Firmware updates are **not** delivered through this API in V1.

## Firmware-side notes (planned, not yet implemented)
Tokens in NVS: ESP32 NVS is not encrypted by default; real protection needs NVS encryption + flash encryption,
which must be enabled deliberately (it is one-way on release eFuse settings). Until then the tokens are
"stored in NVS" but **not** secure against physical flash readout; this limitation is documented here on purpose.
