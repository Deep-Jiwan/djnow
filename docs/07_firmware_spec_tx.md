# 07 — Firmware Specification: Transmitter (TX)

## Target Hardware
- ESP32-C3 Super Mini
- 5x SC6021N B10K 75mm slide potentiometers
- 18650 Li-Ion battery via 3.3V LDO
- Status LED on GPIO 10

---

## Configuration Block (top of file)

```cpp
// ─── USER CONFIG ─────────────────────────────────────────────────
#define BIND_PHRASE               "djnow-default"
#define ESPNOW_CHANNEL            1
#define LED_PIN                   10

#define NUM_SLIDERS               5
const int SLIDER_PINS[NUM_SLIDERS] = {0, 1, 2, 3, 4};  // GPIO / ADC1 channels

#define SLIDER_CHANGE_THRESHOLD   30       // 0-1023 units
#define IDLE_POLL_INTERVAL_US     1000000  // 1 second
#define ACTIVE_WINDOW_MS          120000   // 2 minutes
#define SEND_INTERVAL_MS          100      // 10Hz
#define SCAN_TIMEOUT_MS           60000    // 60 seconds
#define SCAN_SLEEP_US             60000000 // 60 seconds in microseconds
// ─────────────────────────────────────────────────────────
```

---

## Shared Packet Structs (also used by RX)

```cpp
#define DJNOW_MAGIC "DJNOW01"

struct BindAdvertPacket {
    char     magic[8];    // DJNOW_MAGIC
    char     phrase[32];  // bind phrase
    uint8_t  role;        // 0x01 = RX advertising
};

struct BindAckPacket {
    char     magic[8];
    char     phrase[32];
    uint8_t  role;        // 0x02 = TX acknowledging
};

struct DataPacket {
    char     magic[8];
    uint16_t sliders[5];  // 0-1023 each
};
```

---

## State Machine

```cpp
enum TxState {
    STATE_SCANNING,
    STATE_SCAN_SLEEP,
    STATE_IDLE_POLLING,
    STATE_ACTIVE_WINDOW
};
```

---

## setup() Pseudocode

```
1. Serial.begin(115200)  // debug only, not used by deej
2. pinMode(LED_PIN, OUTPUT)
3. analogSetAttenuation(ADC_11db)  // full range on all ADC pins
4. Read all 5 ADC pins into lastSliderValues[]
5. Init WiFi in STA mode, set channel to ESPNOW_CHANNEL
6. Init ESP-NOW
7. Register onDataRecv callback
8. Register onDataSent callback
9. state = STATE_SCANNING
10. scanStartTime = millis()
11. Start broadcasting BindAdvertPacket to FF:FF:FF:FF:FF:FF every 500ms
    (handled in loop via non-blocking timer)
```

---

## loop() Pseudocode

### STATE_SCANNING
```
- Update LED: fast blink 200ms
- Every 500ms: send BindAdvertPacket broadcast
- If rxMacFound flag set by callback:
    - Send BindAckPacket to rxMac
    - Add rxMac as ESP-NOW peer
    - state = STATE_IDLE_POLLING
    - Turn LED OFF
    - Disable WiFi modem
- If millis() - scanStartTime > SCAN_TIMEOUT_MS:
    - state = STATE_SCAN_SLEEP
    - LED: double blink 3 times then OFF
    - esp_wifi_stop()
    - esp_sleep_enable_timer_wakeup(SCAN_SLEEP_US)
    - esp_light_sleep_start()
    - On wake: re-init WiFi, ESP-NOW, state = STATE_SCANNING
```

### STATE_IDLE_POLLING
```
- LED OFF
- esp_wifi_stop()  // modem off
- esp_sleep_enable_timer_wakeup(IDLE_POLL_INTERVAL_US)
- esp_light_sleep_start()
// --- wakes here every 1 second ---
- Read all 5 ADC pins into currentValues[]
- For each slider i:
    if abs(currentValues[i] - lastSliderValues[i]) > SLIDER_CHANGE_THRESHOLD:
        changed = true
        lastSliderValues[i] = currentValues[i]
- If changed:
    - Re-enable WiFi modem
    - Re-init ESP-NOW with saved rxMac peer
    - state = STATE_ACTIVE_WINDOW
    - activeWindowStart = millis()
    - lastChangeTime = millis()
- Else: loop back to light sleep
```

### STATE_ACTIVE_WINDOW
```
- LED OFF
- Every SEND_INTERVAL_MS (100ms):
    - Read all 5 ADC pins
    - For each slider: check change > threshold, update lastSliderValues
    - If any changed: lastChangeTime = millis()
    - Build DataPacket with current lastSliderValues
    - esp_now_send(rxMac, &packet)
- If millis() - lastChangeTime > ACTIVE_WINDOW_MS:
    - state = STATE_IDLE_POLLING
    - esp_wifi_stop()
```

---

## onDataRecv Callback (TX)

```
- Check magic string matches DJNOW_MAGIC
- Check role == 0x01 (RX advertising)
- Check phrase matches BIND_PHRASE
- If all match AND state == STATE_SCANNING:
    - Save sender MAC to rxMac[]
    - Set rxMacFound = true flag
```

---

## ADC Reading

```cpp
uint16_t readSlider(int pin) {
    int raw = analogRead(pin);          // 0-4095 (12-bit)
    int mapped = raw >> 2;              // 0-1023 (10-bit)
    return (uint16_t)constrain(mapped, 0, 1023);
}
```

Call `analogSetAttenuation(ADC_11db)` once in setup for all pins.

---

## Library Dependencies

- `esp_now.h` (built into ESP32 Arduino core)
- `WiFi.h` (built into ESP32 Arduino core)
- `esp_sleep.h` (built into ESP32 Arduino core)
- `esp_wifi.h` (built into ESP32 Arduino core)

No external libraries required.
