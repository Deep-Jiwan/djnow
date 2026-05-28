# 06 — LED Status Codes

Both devices have one status LED. The LED communicates device state without any display or serial monitor needed.

**TX LED pin: GPIO 10**
**RX LED pin: GPIO 2**

---

## Transmitter LED

The TX LED is off as much as possible to conserve power. Once bound it turns off permanently until reboot.

| State | Pattern | Description |
|---|---|---|
| Scanning for RX | Fast blink — 200ms on / 200ms off | Actively looking for RX advertisement |
| Bound | **OFF** | Bound and operating normally. LED off = good. |
| No RX found (60s timeout) | Double blink, 1s pause, repeat | Two quick flashes, pause — RX not found, going to sleep |
| Scan sleep (between retries) | **OFF** | Sleeping 60s before next scan attempt |

### TX LED Pattern Timing
```
Fast blink (scanning):
  ON 200ms → OFF 200ms → repeat

Double blink (no RX found):
  ON 150ms → OFF 150ms → ON 150ms → OFF 150ms → pause 1000ms → repeat

Bound:
  OFF permanently (until reboot)
```

### Why TX LED is OFF when bound
The TX spends most of its life in light sleep. Driving an LED during sleep defeats the purpose of sleep. Once the user sees the LED go off, they know binding succeeded and the device is operating correctly.

---

## Receiver LED

The RX is USB powered so LED usage has no cost. It stays on solid when healthy.

| State | Pattern | Description |
|---|---|---|
| Advertising (no TX bound) | Slow blink — 500ms on / 500ms off | Broadcasting bind phrase, waiting for TX |
| Bound, receiving data | **Solid ON** | TX is connected and sending data |
| Bound, TX went silent (watchdog) | Fast blink — 200ms on / 200ms off | Was bound, lost TX signal, re-advertising |
| Boot / init | OFF briefly (~1s) | Initialising ESP-NOW stack |

### RX LED Pattern Timing
```
Slow blink (advertising):
  ON 500ms → OFF 500ms → repeat

Solid on (bound + receiving):
  ON permanently

Fast blink (lost TX):
  ON 200ms → OFF 200ms → repeat
```

---

## LED Implementation Notes

```cpp
// Non-blocking LED blink using millis()
unsigned long lastLedToggle = 0;
bool ledState = false;

void updateLed(unsigned long interval) {
    if (millis() - lastLedToggle >= interval) {
        lastLedToggle = millis();
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }
}

// Double blink — implement as a small state machine
// states: BLINK1_ON, BLINK1_OFF, BLINK2_ON, BLINK2_OFF, PAUSE
// advance state on timer expiry
```

Do **not** use `delay()` for LED timing — it blocks the main loop and will interfere with ESP-NOW callbacks and serial output on the RX.
