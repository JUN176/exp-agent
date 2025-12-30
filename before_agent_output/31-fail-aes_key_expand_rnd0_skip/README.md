Vulnerability: AES-256 Key-Schedule Skip on rnd == 0

Module: aes (hw/ip/aes)
RTL location: hw/ip/aes/rtl/aes_key_expand.sv (lines 393-423)
Vulnerability ID: 31

Summary
-------
The AES-256 key expansion logic in aes_key_expand.sv gates the first scheduling
step behind the condition (rnd != 0). As a result, when rnd == 0 the
hardware skips the required SubWord/RotWord/Rcon transformation for both the
forward and inverse scheduling paths. This causes the round-1 subkey to be
identical to the round-0 subkey, violating AES-256 semantics and producing
incorrect subkeys for subsequent rounds.

Security Impact
---------------
This defect breaks FIPS-197 compliance for AES-256 and significantly weakens
security by enabling attacks that leverage repeated subkeys (e.g. two-round
meet-in-the-middle, differential/boomerang style attacks). Interoperability
with correct implementations fails (encryption/decryption outputs differ), and
countermeasures depending on correct scheduling (masking, leakage patterns)
are affected.

POC Description
---------------
This package contains a small firmware test (aes_vuln_test.c) that:
- Configures the AES peripheral in CTR mode with a 256-bit key (kKey256).
- Writes the IV (kIv) and a single plaintext block (kPlaintext[0..3]).
- Reads the resulting ciphertext from the AES data out registers.
- Compares the hardware ciphertext against the expected reference
  ciphertext (first 128-bit block from kCtr256Iso9797M2).

If the hardware suffers from the key-schedule bug, the ciphertext will differ
from the reference vector and the test will pass (the POC asserts that a
mismatch occurs). If the hardware is correct, the ciphertext will match and
the test will fail indicating no vulnerability was observed.

Why this triggers the bug
-------------------------
The test feeds a full 256-bit software-provided key to the AES engine and
relies on the hardware to expand it into round subkeys. Because the buggy RTL
skips the SubWord/RotWord/Rcon update for rnd == 0, the next 256-bit window
is not generated and round-1 reuses round-0. This alters the encryption
subkeys and causes incorrect ciphertext when compared to a correct
implementation.

Base library APIs used
----------------------
- mmio_region_from_addr / mmio_region_write32 / mmio_region_read32
- mmio_region_get_bit32
- bitfield_field32_write / bitfield_bit32_write
- memory (memcmp)
- LOG_INFO, CHECK, OTTF test harness

Build & Run
-----------
1. Build the test:
   bazel build //path/to:aes_vuln_test

2. Run (simulation or on hardware):
   bazel test //path/to:aes_vuln_test --test_output=all

Expected Result
---------------
- On vulnerable hardware (or RTL simulation of the buggy revision): the
  hardware ciphertext will NOT match the expected test vector and the test
  will report that the AES-256 key-schedule bug was observed.
- On correct hardware: the hardware ciphertext matches the expected vector and
  the test will fail (assert) because it was written to detect the presence
  of the bug.

Notes & Limitations
-------------------
- The test intentionally avoids using high-level DIF APIs and performs direct
  MMIO register manipulation via the base library to closely mirror register
  sequences.
- The test uses the CTR-mode test vector (kCtr256Iso9797M2) from the
  existing testvectors header to obtain a known-good reference.
- If execution environment or register-field encodings differ, small
  adaptations to the control register field values may be required. The
  approach, however, remains valid: feed a 256-bit key and compare hardware
  output to a known-good vector.
