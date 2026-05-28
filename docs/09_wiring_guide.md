# 09 — Wiring Guide & Pin Connections

## Transmitter Wiring

### ESP32-C3 Super Mini Pin Assignments

| GPIO | Function | Connects to |
|---|---|---|
| GPIO 0 | ADC — Slider 1 (leftmost) | Pot 1 wiper |
| GPIO 1 | ADC — Slider 2 | Pot 2 wiper |
| GPIO 2 | ADC — Slider 3 | Pot 3 wiper |
| GPIO 3 | ADC — Slider 4 | Pot 4 wiper |
| GPIO 4 | ADC — Slider 5 (rightmost, master) | Pot 5 wiper |
| GPIO 10 | Status LED (anode via 330Ω) | LED + resistor |
| 3.3V | Power rail | LDO output, all pot pin 1s |
| GND | Ground rail | LDO GND, all pot pin 3s, LED cathode |

### Potentiometer Wiring (each of 5, identical)

```
SC6021N B10K

Pin 1 (top end)    ►───────────► 3.3V rail
Pin 2 (wiper)      ►───────────► GPIO (0,1,2,3,4)
Pin 3 (bottom end) ►───────────► GND rail
```

> All 5 potentiometers share the same 3.3V and GND rails.
> **Never connect pot pins to 5V — max ADC input is 3.3V.**

### Power Circuit

```
18650 Cell (+) ►──► LDO IN (AMS1117-3.3 or HT7333)
                      LDO OUT ►──► ESP32-C3 3V3 pin
                               ►──► 10µF cap to GND
                               ►──► 100nF cap to GND
                               ►──► All pot pin 1 (3.3V)
18650 Cell (-) ►──► LDO GND ►──► ESP32-C3 GND ►──► All pot pin 3
```

### LED Circuit

```
GPIO 10 ►──► 330Ω resistor ►──► LED anode (+)
                              LED cathode (-) ►──► GND
```

---

## Receiver Wiring

The receiver is minimal — just the ESP32-C3 Super Mini, a USB cable, and a status LED.

### ESP32-C3 Super Mini Pin Assignments

| GPIO | Function | Connects to |
|---|---|---|
| GPIO 2 | Status LED (anode via 330Ω) | LED + resistor |
| USB-C port | Power + CDC Serial | PC USB port |

### LED Circuit

```
GPIO 2 ►──► 330Ω resistor ►──► LED anode (+)
                             LED cathode (-) ►──► GND
```

No other connections needed on the receiver.

---

## Slider Channel Mapping

| Physical Position | GPIO | deej slider index | Default assignment |
|---|---|---|---|
| Leftmost (1) | GPIO 0 | 0 | Unassigned |
| 2nd from left | GPIO 1 | 1 | Unassigned |
| Middle (3rd) | GPIO 2 | 2 | Unassigned |
| 4th from left | GPIO 3 | 3 | Unassigned |
| Rightmost (5th) | GPIO 4 | 4 | Master volume |

Assignments are configured in deej's `config.yaml` on the PC, not in firmware.

---

## Checklist Before Powering On

- [ ] Pot pin 1 connected to 3.3V (NOT 5V)
- [ ] Pot pin 3 connected to GND
- [ ] Pot pin 2 (wiper) connected to correct GPIO
- [ ] LED resistor (330Ω) in series, not shorted
- [ ] LDO input connected to 18650 positive
- [ ] LDO GND connected to 18650 negative
- [ ] Decoupling caps on 3.3V rail
- [ ] TX and RX both flashed with same BIND_PHRASE
- [ ] RX Arduino IDE: USB CDC On Boot = Enabled
- [ ] TX Arduino IDE: USB CDC On Boot = Enabled (for flashing, can disable after)
