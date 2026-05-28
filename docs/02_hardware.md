# 02 — Hardware

## Components

### Transmitter

| Component | Specification | Qty |
|---|---|---|
| Microcontroller | ESP32-C3 Super Mini | 1 |
| Slide Potentiometers | SC6021N B10K 75mm linear single-channel | 5 |
| Battery | 18650 Li-Ion, 2200mAh | 1 |
| Battery holder | Single 18650 with leads | 1 |
| Voltage regulator | AMS1117-3.3 or similar LDO 3.3V | 1 |
| Decoupling capacitors | 10µF + 100nF on 3.3V rail | 1 each |
| Status LED | 3mm or 5mm, any colour | 1 |
| LED resistor | 330Ω | 1 |
| Wiring | Jumper wire / 28AWG hookup | — |
| Enclosure | 3D printed or project box (custom) | 1 |

### Receiver

| Component | Specification | Qty |
|---|---|---|
| Microcontroller | ESP32-C3 Super Mini | 1 |
| Status LED | 3mm or 5mm, any colour | 1 |
| LED resistor | 330Ω | 1 |
| USB cable | USB-C to USB-A (matching host PC) | 1 |

---

## ESP32-C3 Super Mini Pinout Reference

The ESP32-C3 Super Mini exposes the following pins as labelled on the board silkscreen:

```
[ 5V ] [ G ] [ 3.3 ] [ 4 ] [ 3 ] [ 2 ] [ 1 ] [ 0 ]
                     [ 21] [20 ] [10 ] [ 9 ] [ 8 ] [ 7 ] [ 6 ] [ 5 ]
```

Pin labels map to ESP32-C3 GPIO numbers directly (label = GPIO number).

**Built-in LED:** GPIO 8 (on many C3 Super Mini variants — verify on your board, may also be GPIO 2. This project uses GPIO 8 as the configurable LED pin, override in config if needed.)

**Native USB:** The ESP32-C3 Super Mini uses its native USB on GPIO 18/19 internally — exposed via the USB-C port. No UART bridge chip. CDC serial works natively.

---

## ADC Pin Selection — Transmitter

The ESP32-C3 has ADC1 on GPIOs 0–4 and ADC2 on GPIO 5. **ADC2 cannot be used simultaneously with WiFi/ESP-NOW** — this is a hardware limitation of the ESP32-C3.

Therefore all 5 potentiometers must use **ADC1 pins only: GPIO 0, 1, 2, 3, 4**.

| Slider # | Function | GPIO | ADC Channel |
|---|---|---|---|
| 1 (leftmost) | Unassigned | GPIO 0 | ADC1_CH0 |
| 2 | Unassigned | GPIO 1 | ADC1_CH1 |
| 3 | Unassigned | GPIO 2 | ADC1_CH2 |
| 4 | Unassigned | GPIO 3 | ADC1_CH3 |
| 5 (rightmost) | Master / Main Mix | GPIO 4 | ADC1_CH4 |

> **Note:** GPIO 2 is also the status LED pin. In the transmitter, the LED is on GPIO 2 and the pot on GPIO 2 would conflict. **Reassign the TX LED to GPIO 10** instead, keeping all 5 ADC1 pins free for potentiometers.

**Revised TX LED pin: GPIO 10**
**RX LED pin: GPIO 2** (no ADC conflict on receiver)

---

## Potentiometer Wiring

Each SC6021N B10K potentiometer has 3 pins:

```
Pin 1 (top / one end)   ──── 3.3V
Pin 2 (wiper / middle)  ──── GPIO (ADC input)
Pin 3 (bottom / other end) ── GND
```

> **Critical:** Power the potentiometers from **3.3V, not 5V**. The ESP32-C3 ADC maximum input is 3.3V. Applying 5V to an ADC pin will damage the chip.

All 5 potentiometers share the same 3.3V and GND rails. Only the wiper of each connects to its respective GPIO.

---

## Power — Transmitter

```
18650 cell (3.7V nominal, 4.2V full, 3.0V cutoff)
        │
        ▼
  LDO 3.3V regulator (AMS1117-3.3 or HT7333)
        │
        ├──► ESP32-C3 3.3V pin
        ├──► Pot 3.3V rails (all 5 shared)
        └──► Decoupling caps to GND (10µF electrolytic + 100nF ceramic)
```

> **Note on LDO efficiency:** The AMS1117 is not the most efficient LDO but is widely available and simple. For maximum battery life a low-dropout regulator like the HT7333 (45µA quiescent current) is preferred over AMS1117 (5mA quiescent). If sourcing is easy, use HT7333 or MCP1700-3302.

**Do not power the ESP32-C3 from the 18650 directly.** The cell voltage ranges 3.0–4.2V and while technically within USB input range, the ESP32-C3 is rated for 3.0–3.6V on its 3.3V pin. Always use a regulator.

---

## Power — Receiver

The receiver is powered entirely from USB 5V via the USB-C port on the ESP32-C3 Super Mini. The onboard regulator on the Super Mini board handles 5V→3.3V internally. No external regulator needed.

---

## Notes on ESP32-C3 ADC Nonlinearity

The ESP32 ADC is known to be nonlinear, particularly:
- Near 0V: reads higher than actual
- Near 3.3V: reads lower than actual
- Midrange is relatively accurate

Mitigation in firmware:
- Use ESP-IDF ADC calibration (available in Arduino framework via `analogSetAttenuation` and calibration APIs)
- Set attenuation to `ADC_ATTEN_DB_11` (0–3.1V range) for full pot travel
- Remap the calibrated reading to 0–1023 for deej output
- Apply a small deadband at top and bottom (~20 counts) to avoid jitter at extremes
