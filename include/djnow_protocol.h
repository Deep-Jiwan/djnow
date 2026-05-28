#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef BIND_PHRASE
#define BIND_PHRASE "djnow-default"
#endif

namespace djnow {

constexpr char MAGIC[8] = "DJNOW01";
constexpr uint8_t ROLE_RX_ADVERT = 0x01;
constexpr uint8_t ROLE_TX_ACK = 0x02;
constexpr uint8_t SLIDER_COUNT = 5;
constexpr uint8_t ESPNOW_CHANNEL = 1;

// User-requested rank order: GPIO3, GPIO4, GPIO1, GPIO0, GPIO2.
constexpr uint8_t TX_SLIDER_PINS[SLIDER_COUNT] = {3, 4, 1, 0, 2};
constexpr uint8_t TX_LED_PIN = 10;
constexpr uint8_t RX_LED_PIN = 2;

struct BindAdvertPacket {
  char magic[8];
  char phrase[32];
  uint8_t role;
} __attribute__((packed));

struct BindAckPacket {
  char magic[8];
  char phrase[32];
  uint8_t role;
} __attribute__((packed));

struct DataPacket {
  char magic[8];
  uint16_t sliders[SLIDER_COUNT];
} __attribute__((packed));

inline void fillMagic(char out[8]) {
  memcpy(out, MAGIC, sizeof(MAGIC));
}

inline void fillPhrase(char out[32]) {
  memset(out, 0, 32);
  const size_t len = strnlen(BIND_PHRASE, 31);
  memcpy(out, BIND_PHRASE, len);
}

inline bool magicMatches(const char in[8]) {
  return memcmp(in, MAGIC, sizeof(MAGIC)) == 0;
}

inline bool phraseMatches(const char in[32]) {
  return strncmp(in, BIND_PHRASE, 32) == 0;
}

}  // namespace djnow
