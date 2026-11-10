# STM32F429 ETM trace firmware

Bare-metal GCC firmware for the STM32F429I-DISC1 that gives the cortrace rig a
known call tree to trace. It uses CMSIS only (no HAL).

- Clock: 8 MHz HSE, PLL to 60 MHz (HCLK). TRACECLK is HCLK/2 = 30 MHz. The bare
  board has no series resistors on the trace pins, so keep it well below 80 MHz.
- Workload (`work()` then `spin(200)`, forever): `fib(12)` (465 calls), `crunch(200)`
  (200 calls of `leaf`), `branchy(300)`, and `spin` with 255 dependent adds per loop.
  PG13 (LED) toggles once per round.
- The firmware does not touch the trace unit. `cortrace arm --platform stm32f429`
  sets the trace pins, DBGMCU and the CoreSight blocks from the host.

## Build

```sh
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_device_f4 Drivers/cmsis_device_f4
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_core Drivers/cmsis_core
make                 # build/f429_trace.elf
make flash           # OpenOCD, board/stm32f429disc1.cfg
```

`Drivers/` is ignored by git. The Makefile expects the headers in
`Drivers/cmsis_device_f4/Include` and `Drivers/cmsis_core/CMSIS/Core`.

## Trace it

```sh
cortrace arm --platform stm32f429 --os none --probe-telnet 127.0.0.1:<port> --profile coverage
```

See `docs/13-stm32f429.md` in the cortrace repository. Use the 2-bit port.
