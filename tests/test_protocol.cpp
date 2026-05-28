#include <cassert>
#include <cstring>

#include "../include/djnow_protocol.h"

static void testPacketSizes() {
  static_assert(sizeof(djnow::BindAdvertPacket) == 41, "Unexpected advert packet size");
  static_assert(sizeof(djnow::BindAckPacket) == 41, "Unexpected ack packet size");
  static_assert(sizeof(djnow::DataPacket) == 18, "Unexpected data packet size");
}

static void testMagicAndPhraseHelpers() {
  char magic[8] = {};
  djnow::fillMagic(magic);
  assert(djnow::magicMatches(magic));

  char phrase[32] = {};
  djnow::fillPhrase(phrase);
  assert(djnow::phraseMatches(phrase));

  phrase[0] = (phrase[0] == 'x') ? 'y' : 'x';
  assert(!djnow::phraseMatches(phrase));
}

static void testPinOrder() {
  static const uint8_t expected[djnow::SLIDER_COUNT] = {3, 4, 1, 0, 2};
  assert(std::memcmp(djnow::TX_SLIDER_PINS, expected, djnow::SLIDER_COUNT) == 0);
}

int main() {
  testPacketSizes();
  testMagicAndPhraseHelpers();
  testPinOrder();
  return 0;
}
