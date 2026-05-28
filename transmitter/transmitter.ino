#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "../include/djnow_protocol.h"

namespace {

enum class TxState : uint8_t { SCANNING, SCAN_SLEEP, BOUND };

TxState txState = TxState::SCANNING;
uint32_t stateStartedAtMs = 0;
uint32_t lastSampleSentMs = 0;
uint8_t receiverMac[6] = {0};
bool hasReceiverMac = false;

constexpr uint32_t SCAN_TIMEOUT_MS = 60000;
constexpr uint32_t SCAN_SLEEP_MS = 60000;
constexpr uint32_t SEND_INTERVAL_MS = 100;
constexpr uint16_t EDGE_DEADBAND = 20;

bool sameMac(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

void setState(TxState next) {
  txState = next;
  stateStartedAtMs = millis();
}

void setupAnalogInputs() {
  analogReadResolution(12);
  for (uint8_t i = 0; i < djnow::SLIDER_COUNT; ++i) {
    pinMode(djnow::TX_SLIDER_PINS[i], INPUT);
    analogSetPinAttenuation(djnow::TX_SLIDER_PINS[i], ADC_11db);
  }
}

uint16_t scaleAdcToDeej(int raw) {
  if (raw < 0) raw = 0;
  if (raw > 4095) raw = 4095;
  uint16_t value = static_cast<uint16_t>((raw * 1023 + 2047) / 4095);
  if (value < EDGE_DEADBAND) return 0;
  if (value > (1023 - EDGE_DEADBAND)) return 1023;
  return value;
}

void applyLedPattern() {
  const uint32_t elapsed = millis() - stateStartedAtMs;
  if (txState == TxState::SCANNING) {
    digitalWrite(djnow::TX_LED_PIN, (elapsed / 150) % 2);
    return;
  }
  if (txState == TxState::SCAN_SLEEP) {
    const uint32_t phase = elapsed % 1000;
    const bool on = (phase < 90) || (phase > 200 && phase < 290);
    digitalWrite(djnow::TX_LED_PIN, on);
    return;
  }
  digitalWrite(djnow::TX_LED_PIN, LOW);
}

bool setupReceiverPeer() {
  esp_now_del_peer(receiverMac);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = djnow::ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void clearBindingAndRescan() {
  if (hasReceiverMac) {
    esp_now_del_peer(receiverMac);
  }
  memset(receiverMac, 0, sizeof(receiverMac));
  hasReceiverMac = false;
  setState(TxState::SCANNING);
}

bool sendBindAck() {
  djnow::BindAckPacket ack = {};
  djnow::fillMagic(ack.magic);
  djnow::fillPhrase(ack.phrase);
  ack.role = djnow::ROLE_TX_ACK;
  return esp_now_send(receiverMac, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack)) == ESP_OK;
}

void tryBindFromAdvert(const uint8_t* mac, const uint8_t* data, int len) {
  if (txState != TxState::SCANNING || len != static_cast<int>(sizeof(djnow::BindAdvertPacket))) {
    return;
  }
  const auto* advert = reinterpret_cast<const djnow::BindAdvertPacket*>(data);
  if (!djnow::magicMatches(advert->magic) || !djnow::phraseMatches(advert->phrase) ||
      advert->role != djnow::ROLE_RX_ADVERT) {
    return;
  }

  memcpy(receiverMac, mac, 6);
  hasReceiverMac = true;
  if (!setupReceiverPeer()) return;
  if (!sendBindAck()) return;

  setState(TxState::BOUND);
}

void onEspNowReceive(const esp_now_recv_info_t* recvInfo, const uint8_t* data, int len) {
  if (!recvInfo || !recvInfo->src_addr || !data || len <= 0) return;
  tryBindFromAdvert(recvInfo->src_addr, data, len);
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  esp_wifi_set_channel(djnow::ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }
  esp_now_register_recv_cb(onEspNowReceive);
}

void sendSliderPacket() {
  if (!hasReceiverMac || txState != TxState::BOUND) return;

  djnow::DataPacket pkt = {};
  djnow::fillMagic(pkt.magic);
  for (uint8_t i = 0; i < djnow::SLIDER_COUNT; ++i) {
    pkt.sliders[i] = scaleAdcToDeej(analogRead(djnow::TX_SLIDER_PINS[i]));
  }

  const esp_err_t status =
      esp_now_send(receiverMac, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
  if (status != ESP_OK) {
    clearBindingAndRescan();
  }
}

}  // namespace

void setup() {
  pinMode(djnow::TX_LED_PIN, OUTPUT);
  digitalWrite(djnow::TX_LED_PIN, LOW);
  setupAnalogInputs();
  setupEspNow();
  setState(TxState::SCANNING);
}

void loop() {
  applyLedPattern();

  if (txState == TxState::SCANNING && (millis() - stateStartedAtMs) > SCAN_TIMEOUT_MS) {
    setState(TxState::SCAN_SLEEP);
  } else if (txState == TxState::SCAN_SLEEP && (millis() - stateStartedAtMs) > SCAN_SLEEP_MS) {
    setState(TxState::SCANNING);
  } else if (txState == TxState::BOUND && (millis() - lastSampleSentMs) >= SEND_INTERVAL_MS) {
    lastSampleSentMs = millis();
    sendSliderPacket();
  }

  delay(5);
}
