# RF Baseline – CC1310 EasyLink Telemetry System

## Purpose

This document defines the **locked RF baseline configuration** for the CC1310 telemetry system.

The RF layer is treated as a **stable, validated subsystem** and must not be modified during higher-level integration (sensor, protocol, application) unless explicitly required.

---

## RF Stack

- API: EasyLink
- Mode: Asynchronous TX
- Driver: TI Proprietary RF (SimpleLink SDK)
- Platform: CC1310

---

## Operating Characteristics

### Frequency
- Default: from `smartrf_settings`
- Typical: Sub-1 GHz ISM band (e.g. 868 / 915 MHz)

### Transmission Mode
- `EasyLink_transmitAsync()`
- Completion handled via callback + semaphore

### Scheduling

```c
EasyLink_getAbsTime(&absTime);
txPacket.absTime = absTime + EasyLink_ms_To_RadioTime(1000);
TX scheduled ~1 second ahead
Sensor read must occur before this scheduling
Packet Structure (EasyLink Layer)
[Length][Destination Address][Payload]
Length: 1 byte
Address: optional (used as simple filter)
Payload: telemetry data (12 bytes)
Application Payload (Locked)
Length: 12 bytes
Format: defined in telemetry_protocol.h
Encoding: big-endian
No dynamic sizing
TX Architecture
Flow
Sensor read
   ↓
Scaling (fixed-point)
   ↓
TelemetryPacket population
   ↓
Encode → payload buffer
   ↓
EasyLink scheduling
   ↓
Async transmit
Key Rules
No sensor read inside RF callback
No blocking calls in RF callback
Sensor read occurs in TX task only
Packet fully prepared before scheduling TX
RX Architecture
Flow
RF receive (callback)
   ↓
Payload copy
   ↓
Task context processing
   ↓
Decode telemetry
   ↓
UART output
Key Rules
No UART printing in RF callback
No decoding in RF callback
Callback must be fast and deterministic
Timing Constraints
TX Loop
Period: ~1 Hz
Sensor read time: up to ~200 ms (AHT2x worst case)
RF scheduling margin: sufficient
Design Guarantee

Sensor read is completed before:

EasyLink_getAbsTime()

Ensures:

no RF timing drift
no missed transmit windows
Status Handling

RF layer is independent of sensor status.

RF always transmits
Sensor faults reflected only in payload flags
No RF suppression on sensor failure
Error Handling
TX
Callback indicates success/failure
LEDs used for visual feedback
RX
Packet validated before decode
Invalid packets discarded
What is Considered “Locked”

The following must not change during normal development:

EasyLink API usage pattern
Async TX model (callback + semaphore)
RX callback/task separation
Payload length (12 bytes)
Telemetry protocol structure
Scheduling method using EasyLink_getAbsTime()
What Can Change

Allowed modifications:

Sensor integration (TX task only)
Payload field values
Status flags
Higher-level application logic
What Requires New Baseline

Changes that require revisiting RF baseline:

Changing PHY settings
Changing frequency band
Switching to blocking TX
Changing packet length
Adding acknowledgements or retries
Power optimisation affecting RF timing
Validation Criteria

RF baseline is considered valid if:

RX continuously receives packets
Packet counter increments correctly
RSSI is stable
No callback blocking issues
No dropped transmissions under normal conditions

Example:

VER=0x01 TYPE=0x10 ID=0xA1 CNT=64 RSSI=-28 LEN=12
Engineering Principle

The RF layer is treated as a trusted transport layer.

All feature development must respect this boundary:

Application / Sensor Layer
        ↓
Telemetry Protocol
        ↓
RF Transport (LOCKED)
Summary

This RF baseline allows:

stable wireless communication
deterministic scheduling
safe integration of higher-level features

while minimizing risk during iterative development.