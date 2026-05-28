# djnow

**An ESP-NOW based, deej-compatible wireless audio mixer controller.**

djnow lets you control per-application audio volumes on your PC using physical slide potentiometers — completely wirelessly — using two ESP32-C3 Super Mini boards and the ESP-NOW protocol.

It is fully compatible with the [deej](https://github.com/omriharel/deej) desktop client software, meaning no custom client software needs to be written — just use deej as-is.

---

## Overview

| | Transmitter (TX) | Receiver (RX) |
|---|---|---|
| Board | ESP32-C3 Super Mini | ESP32-C3 Super Mini |
| Power | 18650 Li-Ion battery | USB 5V |
| Role | Reads 5 slide pots, sends via ESP-NOW | Receives ESP-NOW, outputs USB Serial to PC |
| Sleep | Light sleep 1s polling + 2 min active window | Always on |
| deej compat | — | Full (CDC serial, pipe-delimited 0-1023 at 10Hz) |

---

## Repository Structure

```
djnow/
├── README.md
├── include/
│   └── djnow_protocol.h       # Shared packet/config definitions
├── transmitter/
│   └── transmitter.ino        # TX firmware
├── receiver/
│   └── receiver.ino           # RX firmware
├── tests/
│   └── test_protocol.cpp      # Host-side protocol/pin tests
└── docs/
    ├── 01_project_overview.md
    ├── 02_hardware.md
    ├── 03_espnow_binding.md
    ├── 04_power_management.md
    ├── 05_deej_protocol.md
    ├── 06_led_status.md
    ├── 07_firmware_spec_tx.md
    ├── 08_firmware_spec_rx.md
    ├── 09_wiring_guide.md
    └── 10_full_conversation.md
```

---

## Quick Start

1. Flash `receiver.ino` to the RX ESP32-C3, with your chosen `BIND_PHRASE` defined
2. Flash `transmitter.ino` to the TX ESP32-C3, with the same `BIND_PHRASE`
3. Power on RX via USB — it will blink indicating it is advertising
4. Power on TX — it will find RX, bind, LED confirms
5. Open deej, point it at the RX serial port, configure your app mappings
6. Done — move sliders, control app volumes wirelessly

---

## Docs

See the [`docs/`](docs/) folder for full technical specifications, wiring diagrams, protocol details, and design decisions.

---

## Pin Mapping (TX Potentiometers)

This implementation uses the requested rank-ordered ADC1 pin set:

- Slider 1 → GPIO3
- Slider 2 → GPIO4
- Slider 3 → GPIO1
- Slider 4 → GPIO0
- Slider 5 → GPIO2

TX status LED uses GPIO10 to keep all five ADC1 pins free.

---

## Local Test Command

```bash
g++ -std=c++17 -Iinclude tests/test_protocol.cpp -o /tmp/djnow_test_protocol && /tmp/djnow_test_protocol
```

---

## License

MIT
