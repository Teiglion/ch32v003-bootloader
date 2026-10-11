# UART Bootloader for CH32V003

[Русский](README.md) | English

For the initial installation, use a WCH-LinkE or a compatible programmer (Ardulink was used during development). Once the bootloader is installed, user application `.bin` files can be programmed via UART.

At power-on, the bootloader checks PC0:
- HIGH - wait for firmware over UART;
- LOW - start the user application.

On the development board used, PC0 is connected to a button.

## Features

- Communication via USART1 at 115200 baud.
- Firmware transfer in blocks of up to 64 bytes.
- Image size validation against the chip's memory capacity.
- CRC32 verification of each block before writing.
- Final CRC32 verification of the image read back from Flash.
- Responses with operation results and error codes.
- Automatic application startup after a successful update.

The bootloader verifies the integrity of the transferred and written data, but not the functional correctness of the application. Automatic rollback to the previous firmware is not supported.

## Memory Layout

| Region | Physical address | Size | Purpose |
|---|---|---|---|
| BOOT/SYSTEM | `0x1FFFF000` | 1920 bytes | Bootloader |
| USER Flash | `0x08000000` | 16 KB | User application |
| RAM | `0x20000000` | 2 KB | RAM |

BOOT/SYSTEM is a separate region: the bootloader does not occupy the beginning of user Flash.
At startup, the selected Flash region is mapped to address `0x00000000`. Therefore, the bootloader linker script uses:

```ld
FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 1920
```

This is the execution address, not the physical address where the bootloader is programmed. To program BOOT/SYSTEM, use the `bootloader` target in `minichlink`. Before programming, edit `MINICHLINK` and `PROGRAMMER` in the Makefile: specify the path to `minichlink`, the programmer to use, and its connection port.
The application image is written starting at `0x08000000`. It must be built to execute with USER Flash mapped to `0x00000000` and include its own startup code.

## Boot Region Configuration

To start the bootloader automatically, booting from BOOT must be selected:

```text
START_MODE = 1
```

Do not change `START_MODE` in the option bytes to boot from USER if you want to use the bootloader. When booting from USER, the application starts directly, bypassing the bootloader and the button check.
If this setting has been changed, restore booting from BOOT using a programmer. Reprogramming the bootloader itself is not necessary if its code is still intact. Writing the binary to BOOT/SYSTEM alone does not replace checking the `START_MODE` setting, since it is stored in the chip's nonvolatile memory.

## UART

| CH32V003 signal | Purpose |
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

## Building

```sh
make clean
make
make flash
```

Build outputs are created in the `build` directory:

```text
build/firmware.elf
build/firmware.bin
build/*.o
```

## Protocol

Communication is binary. Multibyte values are transmitted least significant byte first: little-endian.

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

The `DATA` command is sent once before the first block.

Each block contains:

```text
PAGE_NUMBER  2 bytes
PAYLOAD      up to 64 bytes
PAGE_CRC     4 bytes
```

Numbering starts at zero. Blocks must be transferred strictly in sequence.

The size of the final block is determined by the total image size. Its unused portion is padded with `0xFF` before writing.

The CRC of the final block is calculated only over the payload, excluding padding.

After successfully writing a block, the bootloader responds with:

```text
ACK PAGE_NUMBER
```

### Completion

After all blocks have been transferred, the host sends `END`.

The bootloader calculates the image CRC32 directly from the data in user Flash. Only `IMAGE_SIZE` bytes are included in the calculation.

If the result matches `IMAGE_CRC`, it sends:

```text
DONE 00 00
```

### Response Format

Each response consists of three bytes:

```text
STATUS  VALUE_LOW  VALUE_HIGH
```

For `ACK`, the `VALUE` field contains the block number. For errors, the value depends on where the failure occurred: some responses contain the block number, while others contain zero.

| Code | Status | Meaning |
|---|---|---|
| `0x01` | READY | Flash is ready to receive data |
| `0x02` | ACK | Block verified and written |
| `0x03` | DONE | Entire image verified |
| `0x04` | BAD_PARAMS | Invalid parameters or block number |
| `0x05` | BAD_CRC | CRC mismatch |
| `0x06` | FLASH_ERROR | Flash operation error |
| `0x07` | BAD_STATE | Unexpected command |

### CRC32

The same parameters are used for both blocks and the entire image:

```text
Initial value:        0xFFFFFFFF
Reflected polynomial: 0xEDB88320
Final XOR:            0xFFFFFFFF
```

The CRC is transmitted in little-endian format.

### Error Recovery

After an error or an interrupted transfer, restart the device with PC0 HIGH and repeat the update. Restarting the host program alone does not restart the bootloader.

## Firmware Upload Utility

The `loader/` directory contains the source code for `ch32_loader`,
a utility for transferring firmware to the bootloader over a serial port.
Build it by running `make` in the `loader/` directory.

The utility runs on Linux, uses 115200 baud, and takes two arguments:
the serial port path and the firmware binary file path.

### Uploading Firmware

The device must be in bootloader mode before starting the utility.

Example invocation from the project root:

```sh
./loader/ch32_loader /dev/ttyAMA0 firmware.bin
```

Replace `/dev/ttyAMA0` with the serial port you are using,
and `firmware.bin` with the path to the user application firmware.
If the current user does not have access to the serial port,
the utility can be run with `sudo`.

### Transfer Process

The utility performs the `BEGIN` → `DATA` → `END` sequence described above,
calculates the image and block CRC32 values, and checks the bootloader responses.
The maximum file size accepted by the utility is 16384 bytes.

### Successful Upload Example

```text
Firmware size: 584 bytes
Firmware CRC: 5FC37F25
RX: 01 00 00
READY
RX: 02 00 00
RX: 02 01 00
RX: 02 02 00
RX: 02 03 00
RX: 02 04 00
RX: 02 05 00
RX: 02 06 00
RX: 02 07 00
RX: 02 08 00
RX: 02 09 00
All pages sent
RX: 03 00 00
Flashing successful
```

The `RX:` lines show the received bytes of the device responses.
The `Flashing successful` message is printed after receiving
the expected final `DONE` response.
