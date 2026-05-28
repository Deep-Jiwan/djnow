# 04 — Power Management (Transmitter)

## Overview

The transmitter uses a hybrid light-sleep strategy to maximise battery life while maintaining acceptable responsiveness for a physical slider controller.

There is **no deep sleep** in the main operating loop. Deep sleep was considered but rejected because:
- ESP-NOW requires WiFi stack reinitialisation on every wake from deep sleep (~300ms overhead)
- Light sleep keeps RAM and WiFi stack alive, waking in ~1ms
- Light sleep at 1s intervals still achieves excellent battery life

---

## State Machine

```
┌─────────────────┐
│   BOOT / SCAN   │  Scan for RX up to 60s, then sleep 60s and retry
└────────┬────────┘
         │ bound
         ▼
┌─────────────────┐   no change > 30 for 2 min
│   IDLE POLLING  │◄──────────────────────────────────────┐
│  light sleep    │                                       │
│  wake every 1s  │                                       │
│  read all ADCs  │                                       │
└────────┬────────┘                                       │
         │ any slider delta > 30                          │
         ▼                                                │
┌─────────────────┐                                       │
│  ACTIVE WINDOW  │  send at 10Hz, reset timer on change  │
│  ESP-NOW on     │───────────────────────────────────────┘
│  2 min timeout  │
└─────────────────┘
```

---

## States Detail

### IDLE_POLLING
- ESP32-C3 in light sleep
- Wakes every **1000ms** via `esp_sleep_enable_timer_wakeup(1000000)`
- On wake: reads all 5 ADCs, compares to last known values
- If any slider has changed by more than **30 counts** (0–1023 scale): transition to ACTIVE_WINDOW
- If no change: immediately re-enter light sleep
- WiFi/ESP-NOW radio is **OFF** during idle polling to save power
- LED is **OFF**

### ACTIVE_WINDOW
- WiFi and ESP-NOW radio powered on
- Send DataPacket to RX at **10Hz** (every 100ms) regardless of whether sliders moved
- On each send cycle: read ADCs, update last known values
- Maintain a `lastChangeTime` timestamp
- If any slider changes by >30 counts: reset `lastChangeTime`
- If `millis() - lastChangeTime > 120000` (2 minutes): transition back to IDLE_POLLING
- LED is **OFF** (bound, power saving)

### SCAN_SLEEP (boot only)
- Entered if no RX found within 60s of boot
- Light sleep for 60s
- Wake and restart scan
- LED: double-blink pattern while awake scanning

---

## Threshold and Timing Constants

```cpp
#define SLIDER_CHANGE_THRESHOLD   30      // counts (0-1023) to trigger wake
#define IDLE_POLL_INTERVAL_US     1000000 // 1 second in microseconds
#define ACTIVE_WINDOW_MS          120000  // 2 minutes in milliseconds
#define SEND_INTERVAL_MS          100     // 10Hz
#define SCAN_TIMEOUT_MS           60000   // 60 seconds
#define SCAN_SLEEP_MS             60000   // 60 seconds sleep before retry
```

---

## Estimated Battery Life

Battery: 18650 2200mAh, usable ~1700mAh after regulator losses and aging.

| State | Current | Est. time fraction (casual use) |
|---|---|---|
| Light sleep + 1s ADC wake | ~1.5mA | ~95% |
| Active ESP-NOW window | ~25mA | ~5% |
| **Weighted average** | **~2.7mA** | — |

**Estimated runtime: ~26 days** (casual use, 1–2 active windows per hour)

For heavy use (many active windows per hour), worst case ~10–15 days.

---

## ADC During Light Sleep

The ESP32-C3 ADC **cannot be read during light sleep**. The 1ms wake cycle reads the ADC immediately after waking, before re-entering sleep. This adds negligible average current.

Sequence on each 1s wake:
```
wake → enable ADC → read 5 channels (~2ms) → compare → sleep or activate
```

---

## Radio Power Control

During IDLE_POLLING the WiFi modem is explicitly disabled:
```cpp
esp_wifi_stop();  // or WiFi.disconnect(true) + WiFi.mode(WIFI_OFF)
```

On transition to ACTIVE_WINDOW:
```cpp
WiFi.mode(WIFI_STA);
esp_now_init();
esp_now_add_peer(&rxPeerInfo);
```

This add ~50–100ms latency on the first send after waking from idle, which is imperceptible on a physical slider.
