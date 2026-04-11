# AHT2x Driver Generation — CC1310 (TI Drivers)

Use Codex_Agents/ti_embedded/SKILL.md as the project skill.
Follow the rules in Codex_Agents/ti_embedded/SKILL.md strictly.

## Objective
Create a clean, minimal AHT2x driver for this TI CC1310 CCS project.

## Requirements

### Files to create
- aht2x.h
- aht2x.c

### Driver requirements
- Use TI Drivers I2C (I2C_Handle, I2C_Transaction)
- 7-bit I2C address: 0x38
- Blocking implementation only
- No RTOS complexity

### Functions required
- init (store I2C handle + address)
- read status
- trigger measurement
- wait for measurement complete (poll busy bit with timeout)
- read raw data (6 bytes)
- convert to:
  - temperature (°C)
  - humidity (%RH)

### Code constraints
- Simple and readable
- Compile-friendly for CCS
- No dynamic memory
- No heavy stdio usage

### Error handling
- Status enum
- Timeout handling
- Busy flag validation

## Integration

Update empty.c minimally:
- include aht2x.h
- initialize driver using existing I2C handle
- read sensor once per second
- print results over UART

## Do NOT
- Modify Board files
- Restructure project
- Introduce new dependencies
- Rewrite unrelated code

## Output format
- Show changes as diffs if possible