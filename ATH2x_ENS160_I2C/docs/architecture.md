# Architecture

## Layers

1. Transport Layer
   - TI Drivers (I2C, UART)

2. Device Drivers
   - AHT2x
   - ENS160

3. Application Layer
   - Initialization
   - Sensor polling
   - UART logging

## Design Principles

- Separation of concerns
- Minimal coupling
- Deterministic behaviour
