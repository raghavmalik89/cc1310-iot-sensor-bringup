# System Diagram – CC1310 IoT Telemetry

## Overview

This diagram represents the end-to-end telemetry pipeline.

```
+-------------------+
|   AHT2x Sensor    |
| Temp / Humidity   |
+---------+---------+
          |
          v
+-------------------+
|   I2C Driver      |
+---------+---------+
          |
          v
+-------------------+
|  Sensor Driver    |
|   (AHT2x)         |
+---------+---------+
          |
          v
+-------------------+
| Scaling (×100)    |
| Fixed-point       |
+---------+---------+
          |
          v
+-------------------+
| TelemetryPacket   |
| (12 bytes)        |
+---------+---------+
          |
          v
+-------------------+
| Protocol Encode   |
+---------+---------+
          |
          v
+-------------------+
| EasyLink TX       |
| Sub-1 GHz RF      |
+---------+---------+
          |
          v
~~~~~~~~~ RF LINK ~~~~~~~~~
          |
          v
+-------------------+
| EasyLink RX       |
+---------+---------+
          |
          v
+-------------------+
| Protocol Decode   |
+---------+---------+
          |
          v
+-------------------+
| UART Output       |
| Debug / Logging   |
+-------------------+
```

## Key Principles

- RF layer is fixed baseline
- Sensor integrated at TX only
- RX is decode + validation only
- Fixed-size packet ensures stability
