#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "../include/djnow_protocol.h"

namespace {

enum class RxState : uint8_t { ADVERTISING, BOUND };

RxState rxState = RxState::ADVERTISING;
uint8_t boundTxMac[6] = {0};
bool hasBoundTx = false;
uint32_t lastAdvertMs = 0;
uint32_t lastDataMs = 0;
uint32_t lastSerialMs = 0;
djnow::DataPacket latestData = {};

constexpr uint32_t ADVERT_INTERVAL_MS = 500;
constexpr uint32_t DATA_WATCHDOG_MS = 10000;
constexpr uint32_t SERIAL_INTERVAL_MS = 100;
const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool sameMac(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

void setState(RxState next) {
  rxState = next;
}

void fillDataWithZeros() {
  memset(&latestData, 0, sizeof(latestData));
  djnow::fillMagic(latestData.magic);
}

void addBroadcastPeerIfNeeded() {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
  peerInfo.channel = djnow::ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void addBoundTxPeer() {
  esp_now_del_peer(boundTxMac);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, boundTxMac, 6);
  peerInfo.channel = djnow::ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void clearBinding() {
  if (hasBoundTx) {
    esp_now_del_peer(boundTxMac);
  }
  memset(boundTxMac, 0, sizeof(boundTxMac));
  hasBoundTx = false;
  setState(RxState::ADVERTISING);
}

void sendAdvert() {
  djnow::BindAdvertPacket advert = {};
  djnow::fillMagic(advert.magic);
  djnow::fillPhrase(advert.phrase);
  advert.role = djnow::ROLE_RX_ADVERT;
  esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t*>(&advert), sizeof(advert));
}

void onEspNowReceive(const esp_now_recv_info_t* recvInfo, const uint8_t* data, int len) {
  if (!recvInfo || !recvInfo->src_addr || !data || len <= 0) return;

  if (rxState == RxState::ADVERTISING && len == static_cast<int>(sizeof(djnow::BindAckPacket))) {
    const auto* ack = reinterpret_cast<const djnow::BindAckPacket*>(data);
    if (!djnow::magicMatches(ack->magic) || !djnow::phraseMatches(ack->phrase) ||
        ack->role != djnow::ROLE_TX_ACK) {
      return;
    }
    memcpy(boundTxMac, recvInfo->src_addr, 6);
    hasBoundTx = true;
    addBoundTxPeer();
    lastDataMs = millis();
    setState(RxState::BOUND);
    return;
  }

  if (rxState == RxState::BOUND && hasBoundTx &&
      sameMac(recvInfo->src_addr, boundTxMac) &&
      len == static_cast<int>(sizeof(djnow::DataPacket))) {
    const auto* pkt = reinterpret_cast<const djnow::DataPacket*>(data);
    if (!djnow::magicMatches(pkt->magic)) return;
    memcpy(&latestData, pkt, sizeof(latestData));
    lastDataMs = millis();
  }
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  esp_wifi_set_channel(djnow::ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }
  addBroadcastPeerIfNeeded();
  esp_now_register_recv_cb(onEspNowReceive);
}

void updateLed() {
  if (rxState == RxState::BOUND) {
    digitalWrite(djnow::RX_LED_PIN, HIGH);
    return;
  }
  digitalWrite(djnow::RX_LED_PIN, (millis() / 500) % 2);
}

void emitDeejLine() {
  Serial.printf("%u|%u|%u|%u|%u\n", latestData.sliders[0], latestData.sliders[1],
                latestData.sliders[2], latestData.sliders[3], latestData.sliders[4]);
}

}  // namespace

void setup() {
  pinMode(djnow::RX_LED_PIN, OUTPUT);
  Serial.begin(115200);
  fillDataWithZeros();
  setupEspNow();
  setState(RxState::ADVERTISING);
}

void loop() {
  updateLed();

  if (rxState == RxState::ADVERTISING && (millis() - lastAdvertMs) >= ADVERT_INTERVAL_MS) {
    lastAdvertMs = millis();
    sendAdvert();
  }

  if (rxState == RxState::BOUND && hasBoundTx && (millis() - lastDataMs) > DATA_WATCHDOG_MS) {
    clearBinding();
  }

  if ((millis() - lastSerialMs) >= SERIAL_INTERVAL_MS) {
    lastSerialMs = millis();
    emitDeejLine();
  }

  delay(5);
}
