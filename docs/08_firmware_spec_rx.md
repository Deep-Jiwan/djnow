# 08 — Firmware Specification: Receiver (RX)

## Target Hardware
- ESP32-C3 Super Mini
- USB-C to PC
- Status LED on GPIO 2

---

## Configuration Block (top of file)

```cpp
// ─── USER CONFIG ─────────────────────────────────────────────────
#define BIND_PHRASE               "djnow-default"  // must match TX
#define ESPNOW_CHANNEL            1
#define LED_PIN                   2

#define NUM_SLIDERS               5
#define SERIAL_BAUD               115200
#define SEND_INTERVAL_MS          100     // 10Hz output to deej
#define ADVERT_INTERVAL_MS        500     // broadcast every 500ms
#define RX_WATCHDOG_MS            10000   // 10s no data = re-advertise
// ─────────────────────────────────────────────────────────
```

---

## State Machine

```cpp
enum RxState {
    STATE_ADVERTISING,
    STATE_BOUND
};
```

---

## Global Variables

```cpp
RxState state = STATE_ADVERTISING;
uint8_t txMac[6];
bool    txMacSaved = false;
uint16_t lastSliderValues[5] = {0, 0, 0, 0, 0};
unsigned long lastPacketTime  = 0;
unsigned long lastSerialSend  = 0;
unsigned long lastAdvertTime  = 0;
unsigned long lastLedToggle   = 0;
bool ledState = false;
```

---

## setup()

```
1. Serial.begin(SERIAL_BAUD)      // USB CDC → deej
2. pinMode(LED_PIN, OUTPUT)
3. WiFi.mode(WIFI_STA)
4. WiFi.setChannel(ESPNOW_CHANNEL)
5. esp_now_init()
6. esp_now_register_recv_cb(onDataRecv)
7. state = STATE_ADVERTISING
8. lastAdvertTime = millis()
```

---

## loop()

### Always running (every loop tick)
```
- If millis() - lastSerialSend >= SEND_INTERVAL_MS:
    - sendDeejLine(lastSliderValues)
    - lastSerialSend = millis()
- updateLed()  // non-blocking LED state machine
```

### STATE_ADVERTISING
```
- LED: slow blink 500ms
- If millis() - lastAdvertTime >= ADVERT_INTERVAL_MS:
    - Broadcast BindAdvertPacket to FF:FF:FF:FF:FF:FF
    - lastAdvertTime = millis()
- If txMacSaved flag set by callback:
    - Add txMac as ESP-NOW peer
    - state = STATE_BOUND
    - lastPacketTime = millis()
    - LED: solid ON
    - Stop sending advertisements
```

### STATE_BOUND
```
- LED: solid ON
- If millis() - lastPacketTime > RX_WATCHDOG_MS:
    - Clear txMac, txMacSaved = false
    - Remove TX peer from ESP-NOW
    - state = STATE_ADVERTISING
    - LED: fast blink
    // Note: keep outputting lastSliderValues to deej during re-advertising
```

---

## onDataRecv Callback

```
On any ESP-NOW packet received:

1. Check magic == DJNOW_MAGIC — discard if wrong
2. Check role field:

   role == 0x02 (BindAckPacket from TX):
     - Check phrase == BIND_PHRASE
     - If state == STATE_ADVERTISING:
         - Save src MAC to txMac[]
         - txMacSaved = true

   role == 0x00 (DataPacket, no role byte — identified by struct size):
     OR identify DataPacket by checking packet size == sizeof(DataPacket)
     - Check src MAC == txMac (reject data from unknown sources)
     - Copy sliders[5] into lastSliderValues[5]
     - lastPacketTime = millis()
     - Ensure state == STATE_BOUND
```

> **Note on packet identification:** Since BindAckPacket and DataPacket are different sizes, use `data_len` parameter of the callback to distinguish them before casting.

```cpp
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
    if (data_len == sizeof(BindAckPacket)) {
        // handle bind ack
    } else if (data_len == sizeof(DataPacket)) {
        // handle data
    }
    // else: unknown packet, discard
}
```

---

## Serial Output Function

```cpp
void sendDeejLine(uint16_t sliders[5]) {
    Serial.printf("%d|%d|%d|%d|%d\n",
        sliders[0], sliders[1], sliders[2],
        sliders[3], sliders[4]);
}
```

This is called every 100ms regardless of whether new ESP-NOW data arrived.

---

## LED State Machine (non-blocking)

```cpp
void updateLed() {
    switch(state) {
        case STATE_ADVERTISING:
            // slow blink 500ms
            blinkLed(500);
            break;
        case STATE_BOUND:
            // solid on
            digitalWrite(LED_PIN, HIGH);
            break;
    }
    // fast blink 200ms used during watchdog-triggered re-advertising
    // implement via a separate lostSignal flag if needed
}

void blinkLed(unsigned long interval) {
    if (millis() - lastLedToggle >= interval) {
        lastLedToggle = millis();
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }
}
```

---

## Arduino IDE Board Settings

| Setting | Value |
|---|---|
| Board | ESP32C3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 80MHz (reduces power vs 160MHz) |
| Flash Size | 4MB |
| Partition Scheme | Default |
| Upload Speed | 921600 |

> USB CDC On Boot **must be Enabled** for `Serial` to map to the USB port and appear as a COM port to deej.

---

## Library Dependencies

- `esp_now.h`
- `WiFi.h`
- `esp_wifi.h`

No external libraries required.
