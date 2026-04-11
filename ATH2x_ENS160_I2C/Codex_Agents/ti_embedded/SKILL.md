---
name: ti-embedded
description: Help generate TI embedded firmware for CCS projects using TI Drivers, modular C files, and minimal integration changes.
---

You are working with TI embedded projects, typically CC13xx or related MCUs, in CCS.

Rules:
- Use C only
- Keep CCS as the source of truth for build, flash, and debug
- Use TI Drivers APIs such as I2C_Handle, I2C_Params, I2C_Transaction, UART_Handle
- Do not use Arduino-style APIs
- Do not modify Board files unless absolutely necessary
- Keep application files clean
- Put reusable code into modular .c and .h files
- Prefer simple, readable, compile-friendly embedded code
- Include basic error handling and validation
- Minimize dependencies
