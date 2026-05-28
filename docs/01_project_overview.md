# 01 — Project Overview

## What is djnow?

djnow is a wireless, ESP-NOW based replacement for the wired Arduino hardware used in the [deej](https://github.com/omriharel/deej) project. It allows a user to control per-application audio volumes on a Windows or Linux PC using 5 physical linear slide potentiometers, with no wires between the controller and the PC.

---

## Goals

- Fully wireless transmission between the slider hardware and the PC
- Full compatibility with the deej desktop client software — no custom PC software required
- Maximum power efficiency on the transmitter side (battery powered)
- Simple, self-healing automatic binding between TX and RX using a shared bind phrase
- Status visibility via onboard LEDs with no display required

---

## System Architecture

```
┌─────────────────────────────────┐         ┌──────────────────────────────────┐
│         TRANSMITTER (TX)        │         │          RECEIVER (RX)           │
│         ESP32-C3 Super Mini     │         │          ESP32-C3 Super Mini     │
│         18650 Li-Ion Battery    │         │          USB 5V powered          │
│                                 │         │                                  │
│  5x Slide Pots (B10K, 75mm)    │         │  ESP-NOW → USB CDC Serial        │
│  ADC reads on GPIO 0,1,2,3,4   │  ESP-NOW│  Outputs deej-format string      │
│  Light sleep 1s polling         │────────►│  to PC at 10Hz                  │
│  2 min active TX window         │         │  Always on, self-healing bind    │
│  Deep sleep when idle           │         │                                  │
└─────────────────────────────────┘         └──────────────────────────────────┘
                                                            │ USB Serial (CDC)
                                                            ▼
                                              ┌─────────────────────────┐
                                              │   PC running deej       │
                                              │   Windows / Linux       │
                                              │   Per-app volume ctrl   │
                                              └─────────────────────────┘
```

---

## Key Design Decisions

### Why ESP-NOW?
- No WiFi router required — peer-to-peer
- Extremely fast wake and transmit (sub-millisecond, no association handshake)
- Low power — radio can be off between transmissions
- Range sufficient for desktop/room use (~200m open air, well beyond use case)

### Why deej compatibility?
- deej is a mature, well-supported open source project
- Has a configuration UI, tray icon, process name mapping
- Active community, documentation, and ongoing development
- No reinvention of the wheel needed on the software side

### Why ESP32-C3 Super Mini?
- Native USB (no CH340/CP2102 bridge chip) — the C3 itself is the USB device
- Low power modes available (light sleep ~800µA, deep sleep ~5µA)
- Compact form factor
- Available and inexpensive
- 6 ADC channels on ADC1 — enough for 5 potentiometers

### Why not HID?
Per-application volume control on Windows and Linux requires calling OS audio APIs (WASAPI on Windows, PipeWire/PulseAudio on Linux). These cannot be accessed from outside the machine. A lightweight client-side process (deej) is required. USB HID would only provide master volume control via media keys and was therefore not chosen.

---

## Scope of This Repository

This repository contains:
- Full technical documentation (this docs folder)
- Transmitter firmware (`transmitter/transmitter.ino`)
- Receiver firmware (`receiver/receiver.ino`)
- Wiring and connection guide

This repository does NOT contain:
- The deej client software (see https://github.com/omriharel/deej)
- 3D printable enclosures
- PCB designs (breadboard/perfboard build assumed)

---

## Intended Use

A user sits at their PC. The djnow controller sits on their desk. They reach over and move a slider to adjust game volume, music, Discord, etc. The adjustment is reflected immediately in Windows/Linux per-app volume via deej. The controller runs for months on a single 18650 charge.
