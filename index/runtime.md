# OpenTitan Runtime Library Index Guide

## Overview

The `sw/device/lib/runtime` directory contains core runtime support libraries for OpenTitan device software. This directory provides low-level hardware abstraction, memory protection, interrupt handling, logging, and I/O capabilities.

## Directory Contents

### Memory Protection

#### **epmp.h / epmp.c**
Enhanced Physical Memory Protection (EPMP) library for configuring PMP entries in Machine mode (M-mode). This library manages memory region protection with support for various addressing modes (OFF, TOR, NA4, NAPOT) and locking mechanisms. [1](runtime索引文档.md#4-0) 

Key features:
- Transaction-based configuration API
- Support for Machine Mode Whitelist Policy (MMWP)
- Rule Locking Bypass (RLB) control
- Multiple address matching modes [2](runtime索引文档.md#4-1) 

#### **pmp.h / pmp.c**
Standard RISC-V Physical Memory Protection (PMP) library providing basic PMP region configuration. Supports OFF, NA4, NAPOT, and TOR addressing modes with region locking and permission management. [3](runtime索引文档.md#4-2) 

Key features:
- Region permission configuration (read, write, execute)
- Region locking for M-mode access control
- Support for all standard PMP addressing modes [4](runtime索引文档.md#4-3) 

### Hart (Hardware Thread) Control

#### **hart.h / hart.c**
Core functions for controlling hardware thread execution, including power management and timing utilities. [5](runtime索引文档.md#4-4) 

Key features:
- Wait-for-interrupt (WFI) support
- Instruction cache invalidation
- Microsecond busy-spin delays
- Abort functionality conforming to ISO C11 [6](runtime索引文档.md#4-5) 

#### **hart_polyfills.c**
Platform-specific polyfills for hart functions, providing host-side implementations for functions like `busy_spin_micros` that delegate to standard POSIX functions like `usleep`. [7](runtime索引文档.md#4-6) 

### Processor-Specific Support

#### **ibex.h / ibex.c**
Ibex CPU-specific functions and definitions, including exception handling, cycle counting, and timeout utilities. [8](runtime索引文档.md#4-7) 

Key features:
- Exception type enumeration for Ibex-generated exceptions
- Cycle counter reading with overflow protection
- CSR access functions (mcause, mtval, mepc)
- Timeout initialization and checking utilities
- Convenience macros for timeout-based spinning [9](runtime索引文档.md#4-8) [10](runtime索引文档.md#4-9) 

### Interrupt Management

#### **irq.h / irq.c**
Interrupt control functions for enabling/disabling various interrupt types and configuring the interrupt vector offset. [11](runtime索引文档.md#4-10) 

Key features:
- Interrupt vector offset configuration (256-byte aligned)
- Global interrupt control
- External interrupt control
- Timer interrupt control
- Software interrupt control

### Logging and I/O

#### **log.h / log.c**
Generic logging APIs with severity levels and metadata support. Provides formatted logging with file name and line number information. [12](runtime索引文档.md#4-11) 

Key features:
- Multiple severity levels (Info, Warning, Error, Fatal)
- Automatic file and line number capture
- Format string support compatible with print.h
- DV testbench optimization support
- Macros: LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL [13](runtime索引文档.md#4-12) 

#### **print.h / print.c**
Libc-like printing facilities providing hardware-agnostic formatted output with a subset of standard C printf format specifiers. [14](runtime索引文档.md#4-13) 

Key features:
- Printf-style formatting with buffer_sink_t abstraction
- Standard format specifiers (%d, %u, %x, %s, %c, etc.)
- SystemVerilog format specifiers (%h, %b)
- Custom format specifiers (%!s, %!x, %!b, %r)
- Hexdump utilities
- Configurable stdout sink [15](runtime索引文档.md#4-14) 

#### **print_uart.h / print_uart.c**
UART-specific implementation for print.h stdout functionality. Provides minimal UART driver configuration for console output. [16](runtime索引文档.md#4-15) 

Key features:
- Configures UART for stdout operations
- Uses polled byte transmission
- No flow control (unsafe for interrupt contexts)

### Testing

#### **print_unittest.cc**
C++ unit tests for the print library functionality, testing various format specifiers and output behaviors. [17](runtime索引文档.md#4-16) 

## Notes

This runtime library provides essential low-level services for OpenTitan device software. The EPMP and PMP libraries are particularly important for security-critical applications requiring memory isolation. The logging and printing facilities support both on-device execution and DV testbench environments with optimized code paths for each context.