# CC1310 IoT Sensor Bring-up

This repository documents the development and validation of a CC1310-based IoT sensor node.

## System Overview

- MCU: TI CC1310
- Sensors:
  - AHT2x (Temperature / Humidity)
  - ENS160 (Air Quality)
- Interface: I2C
- Debug: UART output

## Current Status

- [x] I2C bring-up
- [x] AHT2x driver
- [x] ENS160 driver
- [ ] RF communication
- [ ] Power optimisation
- [ ] Custom PCB

## Notes

This repository focuses on development and validation stages.
Production-grade firmware is intentionally not included.
