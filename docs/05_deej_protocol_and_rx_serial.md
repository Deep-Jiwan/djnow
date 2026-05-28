# 05 — deej Protocol & RX Serial Output

## deej Serial Protocol

deej expects a **plain text serial stream** on a COM port (Windows) or `/dev/tty*` (Linux).

### Format
```
<val0>|<val1>|<val2>|<val3>|<val4>\n
```

- Values are **integers, 0–1023** (10-bit range)
- Separated by pipe character `|`
- Terminated by newline `\n`
- Sent continuously at approximately **10Hz** (100ms interval)
- Order matches slider order left to right as configured in deej's `config.yaml`

### Example output line
```
512|0|1023|750|300\n
```

### deej config.yaml mapping (example)
```yaml
slider-mapping:
  0: nircmd.exe        # slider 1 - unassigned, map as needed
  1: nircmd.exe        # slider 2 - unassigned
  2: nircmd.exe        # slider 3 - unassigned
  3: nircmd.exe        # slider 4 - unassigned
  4: master            # slider 5 (rightmost) - master volume

serial:
  com-port: COM3       # match to your RX device port
  baud-rate: 115200

noise-reduction-level: 1
```

---

## RX Serial Implementation

### Baud Rate
```cpp
Serial.begin(115200);
```
Must match deej config exactly. 115200 is the deej default.

### Output on Every Tick (10Hz)
```cpp
// called every 100ms
void sendDeejLine(uint16_t sliders[5]) {
    Serial.printf("%d|%d|%d|%d|%d\n",
        sliders[0], sliders[1], sliders[2],
        sliders[3], sliders[4]);
}
```

### Behaviour When No TX Packet Received
deej will stall or show errors if the serial line goes silent. The RX must **always output at 10Hz** even if no new ESP-NOW packet has arrived.

Strategy:
- RX maintains `lastSliderValues[5]` — the most recently received values
- On every 100ms tick, output `lastSliderValues` regardless
- On ESP-NOW packet received: update `lastSliderValues`, reset RX watchdog
- If watchdog expires (10s no data): RX re-enters advertising but **keeps outputting last known values** to deej — deej stays happy, sliders just freeze at last position until TX reconnects

### Value Mapping
TX sends `uint16_t` values already mapped to 0–1023. RX outputs them directly. No remapping on RX side.

---

## ADC to deej Value Mapping (done on TX)

ESP32-C3 ADC is 12-bit (0–4095). Map to 10-bit (0–1023):

```cpp
uint16_t mapToDeej(int raw) {
    // raw is 0-4095 from analogRead()
    int mapped = raw >> 2;              // 12-bit to 10-bit
    mapped = constrain(mapped, 0, 1023);
    return (uint16_t)mapped;
}
```

For better linearity, use attenuation:
```cpp
analogSetAttenuation(ADC_11db);  // 0-3.1V input range, full pot travel
```

---

## USB CDC on ESP32-C3

The ESP32-C3 Super Mini has **native USB** — the USB-C port is directly connected to the chip's USB peripheral (GPIO 18/19 internally). No CH340 or CP2102 bridge.

In Arduino framework, `Serial` on ESP32-C3 maps to the USB CDC port automatically when using the `Arduino ESP32` board package with USB mode set to **USB-OTG / CDC**.

Board settings in Arduino IDE:
- Board: `ESP32C3 Dev Module`
- USB CDC On Boot: `Enabled`

This makes the device appear as a standard COM port on Windows with no drivers, identical to how deej sees an Arduino.

---

## deej Compatibility Checklist

| Requirement | djnow RX behaviour |
|---|---|
| 115200 baud | ✅ hardcoded |
| 0–1023 values | ✅ mapped on TX, passed through |
| Pipe-delimited | ✅ |
| Newline terminated | ✅ |
| ~10Hz output rate | ✅ 100ms tick |
| Continuous output (no silence) | ✅ repeats last values |
| Standard COM port | ✅ native USB CDC, no drivers |
