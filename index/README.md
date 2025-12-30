# Index Directory Overview

This directory contains index files that map the OpenTitan codebase structure. Use these files to locate specific headers, libraries, and register definitions.

## Available Indices

- **`base.md`**:
    - **Content**: Core low-level libraries (`sw/device/lib/base`).
    - **Key APIs**: MMIO (`abs_mmio`, `mmio`), Memory operations, Bitfield manipulation.
    - **Usage**: **MANDATORY** for all PoC development. This is your primary toolkit.

- **`top.md`**:
    - **Content**: Hardware register definitions (`*_regs.h`) and Device Tables (`dt_*.h`) in `bazel-bin`.
    - **Key APIs**: Register offsets (`AES_KEY_SHARE0_0_REG_OFFSET`), bit masks, and hardware constants.
    - **Usage**: **MANDATORY** for finding register addresses and field definitions.

- **`crypto.md`**:
    - **Content**: Cryptography libraries (`sw/device/lib/crypto`).
    - **Key APIs**: Drivers for AES, HMAC, KMAC, OTBN, Entropy, Key Manager.
    - **Usage**: Consult when the vulnerability involves crypto modules or requires crypto operations (e.g., calculating a hash).

- **`dif.md`**:
    - **Content**: Device Interface Functions (`sw/device/lib/dif`).
    - **Key APIs**: High-level drivers for all peripherals (UART, GPIO, OTP, etc.).
    - **Usage**: **REFERENCE ONLY**. Use these headers to understand hardware behavior, but **DO NOT** use DIF APIs in your PoC code.

- **`lib.md`**:
    - **Content**: General purpose libraries (`sw/device/lib`).
    - **Key APIs**: Arch-specific code, runtime support.
    - **Usage**: Less common, check if you need specific runtime features.

- **`runtime.md`**:
    - **Content**: Runtime support libraries (`sw/device/lib/runtime`).
    - **Key APIs**: Logging (`log.h`), Printing (`print.h`), Hart (core) control.
    - **Usage**: Essential for `LOG_INFO` and outputting test results.

- **`testing.md`**:
    - **Content**: Test frameworks and utilities (`sw/device/lib/testing`).
    - **Key APIs**: OTTF (OpenTitan Test Framework), ISR handling, random data generation.
    - **Usage**: Essential for structuring the test (`OTTF_DEFINE_TEST_CONFIG`, `test_main`).

- **`tests.md`**:
    - **Content**: Existing tests (`sw/device/tests`).
    - **Key APIs**: N/A (Source code).
    - **Usage**: Reference for how to initialize modules or perform specific sequences.

## Recommended Workflow

1.  **Start here**: Read this `README.md` to decide which indices are relevant.
2.  **Mandatory**: Always read `base.md` and `top.md`.
3.  **Specifics**: Read `crypto.md` or `dif.md` based on the target module.
4.  **Framework**: Read `runtime.md` and `testing.md` to set up the test harness.
