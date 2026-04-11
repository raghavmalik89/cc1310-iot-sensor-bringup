Follow the rules in Codex_Agents/ti_embedded/SKILL.md strictly.

You are modifying an existing working CCS project using TI Drivers.

Do NOT rewrite or restructure any existing files.

---

OBJECTIVE:
Create a clean, minimal ENS160 driver.

---

FILES TO CREATE:
- ens160.h
- ens160.c

---

HARD REQUIREMENTS:

1. Use TI Drivers I2C:
   - I2C_Handle
   - I2C_Transaction

2. Device details:
   - I2C address: 0x53

3. Implement ONLY these functions:

   - ENS160_init()
   - ENS160_setMode()
   - ENS160_readStatus()
   - ENS160_readAQI()
   - ENS160_readTVOC()
   - ENS160_readECO2()

4. Mode requirement (IMPORTANT):
   - ENS160 requires setting operating mode before reading
   - Use standard mode (value = 0x02)
   - Add small delay (10–20 ms) after mode set

5. Code constraints:
   - Blocking implementation only
   - No RTOS features
   - No dynamic memory
   - No printf / snprintf
   - No heavy dependencies

6. I2C safety:
   - Only assign writeBuf if writeCount > 0
   - Only assign readBuf if readCount > 0

7. Error handling:
   - Use clear status enum
   - Return error if I2C transfer fails
   - Validate input arguments

---

INTEGRATION (VERY IMPORTANT):

Modify empty.c MINIMALLY:

- Include "ens160.h"
- Initialize ENS160 using existing I2C handle
- Call ENS160_setMode() once after init
- Read AQI, TVOC, eCO2 once per second
- Print values using existing UART helper functions

DO NOT:
- modify UART helpers
- modify AHT2x code
- restructure main loop

---

COMMENTING REQUIREMENT (STRICT):

- Use professional embedded-style comments
- Explain:
  - register purpose
  - magic numbers
  - timing delays
- Keep comments concise and technical
- No unnecessary verbosity

---

OUTPUT FORMAT:

- Show only diffs for:
  - ens160.h
  - ens160.c
  - changes to empty.c

---

DO NOT GUESS:
If unsure about a register or value, leave a TODO comment instead of guessing.