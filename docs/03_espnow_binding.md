# 03 — ESP-NOW Binding Scheme

## Concept

Both devices are flashed with an identical `BIND_PHRASE` string defined at compile time. This phrase is used to identify compatible djnow peers and reject unrelated ESP-NOW traffic.

There is no pairing button, no manual configuration, and no stored MAC addresses persisted to flash. Binding happens automatically on every boot and is entirely self-healing.

---

## Bind Phrase

Defined in both firmware files as:

```cpp
#define BIND_PHRASE "djnow-default"
```

Change this string to any value before flashing both devices. Devices with mismatched phrases will ignore each other entirely.

---

## Roles

| Device | Role in binding |
|---|---|
| RX (Receiver) | **Advertiser** — broadcasts its presence and bind phrase |
| TX (Transmitter) | **Scanner** — listens for a matching advertiser, then binds |

The RX advertises because it is USB-powered and always on. This costs the TX nothing in terms of battery — it only wakes and scans when it needs to bind.

---

## Binding Packet Structure

The RX broadcasts a binding advertisement packet on the ESP-NOW broadcast address (`FF:FF:FF:FF:FF:FF`).

```cpp
struct BindAdvertPacket {
    char     magic[8];       // "DJNOW01\0" — fixed magic string
    char     phrase[32];     // bind phrase, null terminated
    uint8_t  role;           // 0x01 = RX advertising
};
```

The TX sends a bind acknowledgement packet directly back to the RX MAC once it finds a matching advertisement:

```cpp
struct BindAckPacket {
    char     magic[8];       // "DJNOW01\0"
    char     phrase[32];     // echo back the bind phrase
    uint8_t  role;           // 0x02 = TX acknowledging
};
```

Data packets sent after binding:

```cpp
struct DataPacket {
    char     magic[8];       // "DJNOW01\0"
    uint16_t sliders[5];     // 5 slider values, 0-1023 each
};
```

The magic string on all packets lets both sides quickly discard unrelated ESP-NOW traffic from other devices in range.

---

## Binding Flow — Step by Step

### RX Boot Sequence
```
1. Boot
2. Init ESP-NOW
3. Register receive callback
4. Enter ADVERTISING state
5. Every 500ms: broadcast BindAdvertPacket to FF:FF:FF:FF:FF:FF
6. LED: slow blink (advertising)

7. On receive callback:
   a. Check magic string — discard if wrong
   b. Check role == 0x02 (TX ack)
   c. Check phrase matches BIND_PHRASE
   d. If all match: save TX MAC address, enter BOUND state
   e. Stop broadcasting advertisements
   f. LED: solid on (bound)

8. In BOUND state:
   - Accept DataPackets only from saved TX MAC
   - Reset 10s watchdog timer on each valid packet received
   - If watchdog expires (no data for 10s): clear TX MAC, return to ADVERTISING state
```

### TX Boot Sequence
```
1. Boot
2. Init ESP-NOW
3. Register receive callback
4. Enter SCANNING state
5. LED: fast blink (scanning)
6. Start 60s scan timeout timer

7. Listen for incoming BindAdvertPackets:
   a. Check magic string — discard if wrong
   b. Check role == 0x01 (RX advertising)
   c. Check phrase matches BIND_PHRASE
   d. If all match: save RX MAC address

8. Send BindAckPacket to saved RX MAC

9. Wait for implicit confirmation (begin receiving no error on send)
   - If send succeeds: enter BOUND state
   - LED: OFF (bound, power saving)

10. If 60s scan timeout expires without finding RX:
    - Enter SCAN_SLEEP state
    - LED: double blink pattern
    - Sleep for 60s (light sleep)
    - Wake and restart from step 3

11. In BOUND state:
    - Run normal sleep/wake/send cycle (see power management doc)
    - Never re-scan unless rebooted
```

---

## Self-Healing Behaviour

| Scenario | What happens |
|---|---|
| TX reboots | TX scans → RX watchdog expired or still bound → RX re-advertises after 10s → TX finds it → re-binds |
| RX reboots | RX starts advertising → TX is asleep → TX wakes on 1s cycle, has saved MAC, tries to send → send fails → TX has no recovery path without reboot. **Therefore:** TX should also reboot or be power cycled if RX is rebooted. This is acceptable — RX is USB powered and rarely reboots. |
| Interference / missed packets | RX watchdog is 10s — TX sends at 10Hz during active window, so watchdog only triggers after 100 consecutive missed packets. Very robust. |
| Wrong bind phrase device nearby | Magic + phrase check discards all packets immediately |

---

## ESP-NOW Channel

Both devices must operate on the **same WiFi channel**. Since no router is involved, the channel is hardcoded:

```cpp
#define ESPNOW_CHANNEL 1
```

Both TX and RX set this channel explicitly. Do not rely on defaults.

---

## Broadcast vs Unicast

- **Advertising** uses broadcast (`FF:FF:FF:FF:FF:FF`) — no peer registration needed
- **Data packets** use unicast to the bound peer MAC — requires `esp_now_add_peer()` with the saved MAC
- After binding, broadcast is no longer used by either device
