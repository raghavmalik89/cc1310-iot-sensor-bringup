# CC1310 RF Telemetry Transmitter (TX)

This project implements the transmitter (TX) side of a CC1310-based IoT telemetry system using EasyLink.

It reads sensor data, encodes it into a fixed telemetry packet, and transmits it over Sub-1 GHz RF.

---

## Function

- Reads AHT2x sensor (temperature and humidity) over I2C
- Converts values to fixed-point format (×100 scaling)
- Packs data into a 12-byte telemetry packet
- Transmits using EasyLink (async mode)

---

## System Role

```
Sensor → TX → RF → RX → UART
```

This project is the **data source** in the system.

---

## Sensor Integration

- Sensor: AHT2x  
- Interface: I2C  
- Driver: `aht2x.c / aht2x.h`

### Initialization

- I2C initialized in TX task context  
- AHT2x initialized and calibrated once at startup  

### Read Flow

```
AHT2x_readTemperatureHumidity()
→ scale values
→ populate TelemetryPacket
→ encode
→ transmit
```

---

## Telemetry Format

- Protocol Version: `0x01`
- Packet Type: `0x10`
- Payload Length: `12 bytes`

---

## Status Flags

- SENSOR_DATA_VALID
- AHT2X_READ_FAULT

---

## Timing Model

- TX loop ~1 Hz
- Sensor read occurs before RF scheduling

---

## Example

```
TEMP=2630 → 26.30°C
HUM=6065 → 60.65%
FLAGS=0x01
```

---

## Notes

- RF layer is preserved as baseline
- Sensor integrated minimally into TX flow
