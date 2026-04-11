CC1310 IoT Sensor Bring-up

This repository documents the structured development and validation of a CC1310-based IoT sensor node, progressing from hardware bring-up to a working wireless telemetry system.

System Overview
MCU: TI CC1310 (Sub-1 GHz RF)
Sensors:
AHT2x (Temperature / Humidity) ✅ Integrated
ENS160 (Air Quality) ⏳ Planned
Interface: I2C
Wireless: EasyLink (Sub-1 GHz RF)
Debug: UART (RX-side telemetry output)
Current Status
Core Bring-up
 GPIO + UART validation
 I2C bring-up
 AHT2x driver (standalone validation)
 ENS160 driver (standalone validation)
RF System
 EasyLink TX/RX link established
 Async RX callback architecture
 Stable UART telemetry output (RX side)
Protocol
 Fixed 12-byte telemetry protocol defined
 Big-endian encoding implemented
 RX decode + validation working
Sensor Integration
 AHT2x integrated into RF TX pipeline
 Real-time temperature and humidity telemetry
 Status flag handling (valid / fault)
 End-to-end validation (Sensor → RF → RX)
Remaining Work
 Battery measurement integration
 ENS160 integration (new packet type)
 Power optimisation (sleep / duty cycle)
 Custom PCB design
Telemetry System

The system now implements a complete embedded telemetry pipeline:

Sensor (AHT2x)
   ↓
I2C Driver
   ↓
Fixed-point scaling (×100)
   ↓
Telemetry Packet (12 bytes, locked format)
   ↓
RF Transmission (EasyLink)
   ↓
RX Decode + UART Output
Example Output (RX)
VER=0x01 TYPE=0x10 ID=0xA1 FLAGS=0x01 CNT=64 TEMP=2630 HUM=6065 BAT=0 RSSI=-28 LEN=12
TEMP=2630 → 26.30°C
HUM=6065 → 60.65% RH
Design Principles
Minimal, controlled changes during bring-up
RF transport treated as a fixed baseline
Protocol kept stable and inspectable
Sensor drivers integrated as reusable modules
Clear separation of:
sensor acquisition
data scaling
protocol encoding
RF transport
Repository Scope

This repository focuses on:

Bring-up and validation
System integration
RF telemetry development
Embedded architecture decisions

It is intentionally not a polished production SDK.

Next Steps
Add battery telemetry
Extend protocol for ENS160 (air quality)
Implement power-saving modes
Transition to custom PCB