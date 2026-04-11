# RF Telemetry Protocol Specification
# Project: CC1310 EasyLink Sensor Telemetry
# Protocol Version: 0x01

---

## 1. Purpose

This document defines the **application-layer telemetry packet format** used between:

- `rfEasyLinkEchoTx` (transmitter)
- `rfEasyLinkEchoRx` (receiver)

This is the **authoritative source of truth** for packet structure.

If implementation and this document disagree:
→ Implementation must be updated to match this document unless explicitly instructed otherwise.

---

## 2. Scope

This protocol defines:
- payload structure
- field encoding
- status flags
- compatibility rules

This protocol does NOT define:
- RF PHY configuration
- EasyLink configuration
- frequency / modulation settings
- callback architecture

These are covered separately in:
→ `docs/rf-baseline.md`

---

## 3. RF Baseline Reference

The protocol assumes a **locked RF transport layer**.

Key parameters:

- Frequency: 868 MHz (example baseline)
- Symbol rate: 50 kBaud
- Deviation: 25 kHz
- RX bandwidth: 98 kHz
- TX power: 14 dBm
- Sync word: 0x930B51DE
- Whitening: disabled
- CRC: enabled

⚠️ RF settings must not be changed during protocol integration.

---

## 4. Packet Philosophy

Design goals:

- Human-readable via UART logs
- Deterministic decode on RX
- Fixed-length structure during bring-up
- Compact and efficient
- Embedded-safe (no floats on-air)
- Extensible without breaking architecture

---

## 5. Packet Types

| Value | Type       | Description            |
|------|------------|------------------------|
| 0x10 | Telemetry  | Sensor data (current)  |
| 0x20 | Status     | System state (future)  |
| 0x30 | Debug      | Debug info (future)    |
| 0x40 | Reserved   | Config/response        |

Current implementation uses:
→ **0x10 (telemetry only)**

---

## 6. Telemetry Packet Layout

### Payload Length: 12 bytes

| Byte | Field                  |
|------|------------------------|
| 0    | protocol_version       |
| 1    | packet_type            |
| 2    | device_id              |
| 3    | status_flags           |
| 4    | packet_counter_hi      |
| 5    | packet_counter_lo      |
| 6    | temperature_hi         |
| 7    | temperature_lo         |
| 8    | humidity_hi            |
| 9    | humidity_lo            |
| 10   | battery_mv_hi          |
| 11   | battery_mv_lo          |

---

## 7. Field Definitions

### 7.1 protocol_version
- Size: 1 byte
- Current: `0x01`
- Purpose: detect incompatible changes

---

### 7.2 packet_type
- Size: 1 byte
- Telemetry value: `0x10`
- Purpose: allow multiple packet families

---

### 7.3 device_id
- Size: 1 byte
- Purpose: node identity
- Note: not a globally unique ID

---

### 7.4 status_flags

- Size: 1 byte

| Bit | Meaning                         |
|-----|---------------------------------|
| 0   | sensor data valid               |
| 1   | AHT2x read fault                |
| 2   | ENS160 read fault               |
| 3   | battery measurement valid       |
| 4   | low battery warning             |
| 5   | watchdog reset detected         |
| 6   | reserved                        |
| 7   | reserved                        |

Current implementation uses:
- bit 0 (valid)
- bit 1 (AHT2x fault)

---

### 7.5 packet_counter

- Size: 2 bytes
- Type: `uint16`
- Encoding: big-endian
- Purpose:
  - detect packet loss
  - observe resets
  - ordering

Wrap:
- 65535 → 0

---

### 7.6 temperature

- Size: 2 bytes
- Type: `int16`
- Unit: 0.01 °C
- Encoding: big-endian

Examples:
- 25.34°C → 2534
- -3.25°C → -325

---

### 7.7 humidity

- Size: 2 bytes
- Type: `uint16`
- Unit: 0.01 %RH
- Encoding: big-endian

Example:
- 48.75% → 4875

---

### 7.8 battery_mv

- Size: 2 bytes
- Type: `uint16`
- Unit: mV
- Encoding: big-endian

Current phase:
- not implemented
- value = `0x0000`

---

## 8. Endianness

All multi-byte fields are:

→ **big-endian on-air**

Reason:
- easier debugging
- consistent interpretation
- deterministic decoding

---

## 9. Compatibility Rules

### Compatible changes
(no version change required)

- internal refactoring
- documentation updates
- UART formatting changes

---

### Incompatible changes
(require protocol version increment)

- field position changes
- field size changes
- scaling changes
- signed/unsigned changes
- endianness changes

---

## 10. TX Rules

TX must:

- populate data via `TelemetryPacket`
- use `telemetryProtocolEncodeTelemetry()`
- not manually assemble payload bytes
- keep structure synchronized with RX
- preserve RF transport baseline

---

## 11. RX Rules

RX must:

- decode using shared protocol
- validate version and type
- not process inside RF callback
- handle unknown packet types safely

---

## 12. Error Handling Philosophy

- RF always transmits
- Sensor errors reflected in flags only
- No suppression of RF due to sensor failure
- Invalid data replaced with safe defaults

---

## 13. Future Extensions

Planned additions:

- ENS160 data (AQI, TVOC, eCO2)
- battery measurement
- reset cause
- firmware version

These require:
- new packet types OR
- protocol version update

---

## 14. Default Values

Unless overridden:

- protocol_version = `0x01`
- packet_type = `0x10`
- device_id = project-defined
- battery_mv = `0x0000`