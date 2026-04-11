# CC1310 IoT Sensor Node Architecture

## Purpose

This repository documents the staged development of a CC1310-based wireless sensor node.

The project is being built in controlled bring-up phases:

1. peripheral validation
2. sensor driver validation
3. RF link validation
4. protocol definition
5. sensor-to-RF integration
6. future power and hardware optimisation

The current goal is a simple, inspectable, low-risk telemetry system rather than a production SDK.

---

## System Overview

The node measures sensor data locally, encodes it into a fixed telemetry frame, and transmits it over Sub-1 GHz RF using EasyLink.

### Current implemented data path

```text
AHT2x sensor
   ↓
I2C driver
   ↓
AHT2x driver module
   ↓
Scaling to fixed-point telemetry units
   ↓
TelemetryPacket population
   ↓
telemetryProtocolEncodeTelemetry()
   ↓
EasyLink transmit
   ↓
RF receive
   ↓
telemetryProtocolDecodeTelemetry()
   ↓
UART output on RX