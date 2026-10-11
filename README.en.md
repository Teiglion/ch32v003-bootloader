# UART Bootloader for CH32V003

[Русский](README.md) | English

For the initial installation, use a WCH-LinkE or a compatible programmer (Ardulink was used during development). Once the bootloader is installed, application `.bin` files can be uploaded over UART.

At power-on, the bootloader checks PC0:
- HIGH - wait for a firmware upload over UART;
- LOW - start the user application.

On the development board used for this project, PC0 is connected to a button.

## Features

- Communication over USART1 at 115200 baud.
- Firmware transfer in blocks of up to 64 bytes.
- Image size validation against the available Flash capacity.
- CRC32 verification of each block before writing.
- Final CRC32 verification of the image read back from Flash.
- Responses reporting operation results and error codes.
- Automatic application startup after a successful update.

The bootloader verifies the integrity of the transferred and written data, but not the functional correctness of the application. Automatic rollback to the previous firmware is not implemented.

## Memory Layout

| Region | Physical address | Size | Purpose |
|---|---|---|---|
| BOOT/SYSTEM | `0x1FFFF000` | 1920 bytes | Bootloader |
| USER Flash | `0x08000000` | 16 KiB | User application |
| RAM | `0x20000000` | 2 KiB | Working memory |

BOOT/SYSTEM is a separate region: the bootloader does not occupy the beginning of USER Flash.

At startup, the selected Flash region is mapped to `0x00000000`. Therefore, the bootloader linker script uses:

```ld
FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 1920
```

This is the execution address, not the physical installation address. To write to BOOT/SYSTEM, the `bootloader` destination is used in `minichlink`.

Before flashing, edit `MINICHLINK` and `PROGRAMMER` in the Makefile to specify your path to `minichlink`, programmer, and connection port.

The application image is written starting at `0x08000000`. It must be built to execute with USER Flash mapped to `0x00000000` and include its own startup code.

## Startup Region Configuration

For the bootloader to start automatically, startup from BOOT must be selected:

```text
START_MODE = 1
```

Do not change `START_MODE` in the option bytes to start from USER if you want to use the bootloader. Starting from USER launches the application directly, bypassing the bootloader and the button check.

If this setting has been changed, restore startup from BOOT using a programmer. Rewriting the bootloader itself is unnecessary if its code is intact. Writing the binary to BOOT/SYSTEM alone does not replace checking `START_MODE`, since this setting is stored in the chip's nonvolatile memory.

## UART

| CH32V003 signal | Function |
|---|---|
| PD5 | USART1 TX |
| PD6 | USART1 RX |

Connect UART with TX and RX crossed:

```text
Adapter TX  --> PD6 / MCU RX
Adapter RX  <-- PD5 / MCU TX
GND         --- GND
```

Communication settings:

```text
115200 baud
8 data bits
No parity
1 stop bit
No hardware flow control
```

## Build

```sh
make clean
make
make flash
```

Build outputs are placed in the `build` directory:

```text
build/firmware.elf
build/firmware.bin
build/*.o
```

## Protocol

Communication is binary. Multibyte integers are transmitted least significant byte first: little-endian.

### Host Commands

| Code | Name | Purpose |
|---|---|---|
| `0x01` | BEGIN | Start an update |
| `0x02` | DATA | Start block transfer |
| `0x03` | END | Finish the transfer |

### Header

```text
BEGIN       1 byte
IMAGE_SIZE  4 bytes
IMAGE_CRC   4 bytes
```

The bootloader validates the image size and erases the required Flash regions. Once preparation succeeds, it sends:

```text
READY 00 00
```

### Block Transfer

The `DATA` command is sent once, before the first block.

Each block contains:

```text
PAGE_NUMBER  2 bytes
PAYLOAD      up to 64 bytes
PAGE_CRC     4 bytes
```

Block numbering starts at zero. Blocks must be transmitted in sequence.

The size of the final block is determined by the total image size. Its unused portion is filled with `0xFF` before writing.

The CRC of the final block is calculated over the actual payload only, excluding the padding.

After successfully writing a block, the bootloader responds:

```text
ACK PAGE_NUMBER
```

### Completion

After sending all blocks, the host sends `END`.

The bootloader calculates the image CRC32 directly from USER Flash. Only `IMAGE_SIZE` bytes are included in the calculation.

If the result matches `IMAGE_CRC`, it sends:

```text
DONE 00 00
```

### Response Format

Each response consists of three bytes:

```text
STATUS  VALUE_LOW  VALUE_HIGH
```

For `ACK`, the `VALUE` field contains the block number. For errors, its meaning depends on where the failure occurred: some responses contain a block number, while others contain zero.

| Code | Status | Meaning |
|---|---|---|
| `0x01` | READY | Flash is prepared for the transfer |
| `0x02` | ACK | Block verified and written |
| `0x03` | DONE | Entire image verified |
| `0x04` | BAD_PARAMS | Invalid parameters or block number |
| `0x05` | BAD_CRC | CRC mismatch |
| `0x06` | FLASH_ERROR | Flash operation failed |
| `0x07` | BAD_STATE | Unexpected command |

### CRC32

The same parameters are used for individual blocks and the entire image:

```text
Initial value:      0xFFFFFFFF
Reflected polynomial: 0xEDB88320
Final XOR:          0xFFFFFFFF
```

CRC values are transmitted in little-endian format.

### Error Recovery

After an error or an interrupted transfer, restart the device with PC0 held HIGH and upload the firmware again. Restarting the host program alone does not restart the bootloader.
