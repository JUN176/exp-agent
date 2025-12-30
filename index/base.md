# Files in `sw/device/lib/base` Directory

The `sw/device/lib/base` directory contains fundamental low-level libraries for the OpenTitan device software. Here's a comprehensive overview of all files and their purposes:

## Core Memory-Mapped I/O (MMIO)

**`abs_mmio.h` / `abs_mmio.c`**: Absolute Memory-mapped IO functions for volatile access to hardware registers, specifically designed for ROM and ROM Extension production libraries. 

**`mmio.h` / `mmio.c`**: Memory-mapped IO functions with an opaque region handle (`mmio_region_t`), providing either volatile accesses or instrumentation calls for testing. 

## Bitfield Manipulation

**`bitfield.h` / `bitfield.c`**: Bitfield manipulation functions for reading/writing specific bits and fields within 32-bit values, including operations like find-first-set, count leading/trailing zeros, popcount, and byte swapping.  

## Security and Hardening

**`hardened.h` / `hardened.c`**: Data types and operations for hardened code, including hardened boolean types with high Hamming distance, laundering operations to prevent compiler optimizations that could compromise security, constant-time operations, and hardened check macros.  

**`hardened_asm.h`**: Assembly-compatible definitions for hardened boolean values that can be used in both C and assembly code. 

**`hardened_memory.h` / `hardened_memory.c`**: Hardened memory operations for constant-power buffer manipulation to mitigate power-analysis attacks, including secure copy, compare, XOR, and shredding operations. 

**`multibits.h`**: Multi-bit boolean values that provide redundancy to configuration fields, making it difficult to fault values to a "good" state. 

**`multibits_asm.h`**: Assembly-compatible versions of multi-bit boolean values. 

## Status and Error Handling

**`status.h` / `status.c`**: Status code system based on absl_status codes, with packed bitfields encoding error location (module ID and line number) for lightweight stack traces. 

## Memory Operations

**`memory.h` / `memory.c`**: OpenTitan device memory library providing memory functions for aligned word accesses and common C library string.h functions like `memcpy`, `memset`, `memcmp`, `memchr`, and `memrchr`. 

## Control and Status Registers (CSR)

**`csr.h`**: Ibex Control and Status Register (CSR) interface providing macros for read, write, set bits, and clear bits operations on CSRs. 

**`csr_registers.h`**: Definitions of Ibex CSR addresses as constants. 

## Utility Functions

**`macros.h`**: Generic preprocessor macros including `ARRAYSIZE`, inline directives, variadic argument counting, concatenation, static assertions, and platform-specific attributes. 

**`adv_macros.h`**: Advanced preprocessor techniques for conditional compilation and macro expansion. 

**`stdasm.h`**: Simple header providing the `asm` keyword in analogy with ISO C headers.  

**`math.h` / `math.c`**: Math helper functions including 64-bit division (`udiv64_slow`) and ceiling division. 

**`math_builtins.c`**: Additional math builtin implementations.

**`crc32.h` / `crc32.c`**: CRC32 computation functions implementing IEEE 802.3 CRC-32 (compatible with Python's `zlib.crc32()`). 

**`random_order.h` / `random_order.c`**: Functions for generating random traversal orders, important for constant-power code to mitigate side-channel attacks. 

## Testing Support

**`global_mock.h`**: Base class for globally-accessible mock objects used in unit tests, ensuring only one instance exists at a time. 

**Mock Files**: `mock_abs_mmio.h/.cc`, `mock_mmio.h/.cc`, `mock_crc32.h/.cc` - Mock implementations for testing.

## Test Files

- `*_unittest.cc` - C++ unit tests
- `*_functest.c` - Functional tests
- `*_perftest.c` - Performance tests

Test files include: `bitfield_unittest.cc`, `crc32_unittest.cc`, `crc32_functest.c`, `crc32_perftest.c`, `global_mock_unittest.cc`, `hardened_unittest.cc`, `hardened_functest.c`, `hardened_memory_unittest.cc`, `math_unittest.cc`, `math_builtins_unittest.cc`, `memory_unittest.cc`, `memory_perftest.c`, `mock_mmio_test.cc`, `mock_mmio_test_utils.h`, `random_order_unittest.cc`, `status_unittest.cc`, `status_report_unittest.cc`, `status_report_unittest_c.c/.h`

## Subdirectories

**`freestanding/`**: Contains freestanding C library headers (assert.h, float.h, iso646.h, limits.h, stdalign.h, stdarg.h, stdbool.h, stddef.h, stdint.h, stdnoreturn.h, string.h) that provide minimal standard library support without requiring a full libc implementation. 

**`internal/`**: Contains internal implementation headers:
- `absl_status.h` - Internal status code definitions
- `status.h` - Internal status implementation details

## Notes

This directory provides the foundational building blocks for all OpenTitan device software, with a strong emphasis on security through hardening techniques, side-channel attack mitigation, and careful memory management. The library is designed to work in a freestanding environment without relying on a full C standard library.
