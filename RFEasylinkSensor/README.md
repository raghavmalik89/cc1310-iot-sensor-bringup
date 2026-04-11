# RF EasyLink Sensor System

This folder contains the working RF telemetry system built on TI CC1310 using EasyLink.

## Components

### Transmitter (TX)
- Path: `rfEasyLinkEchoTx/`
- Reads sensor data (AHT2x)
- Encodes telemetry packet
- Transmits over Sub-1 GHz RF

### Receiver (RX)
- Path: `rfEasyLinkEchoRx/`
- Receives RF packets
- Decodes telemetry
- Outputs data over UART

---

## Current Functionality

- Stable EasyLink TX/RX communication
- Fixed 12-byte telemetry protocol
- Real-time AHT2x temperature and humidity transmission
- RX-side UART output for validation

---

## Data Flow

```text
Sensor (AHT2x)
   ↓
TX (encode + RF transmit)
   ↓
RF link
   ↓
RX (decode)
   ↓
UART output