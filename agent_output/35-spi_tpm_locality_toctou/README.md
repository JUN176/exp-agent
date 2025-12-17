Vulnerability: TPM locality TOCTOU in spi_device (spi_tpm.sv)

Overview
--------
This exploit package demonstrates the Time-Of-Check/Time-Of-Use (TOCTOU)
flaw in hw/ip/spi_device/rtl/spi_tpm.sv (lines 805-810). The RTL uses
non-blocking assignments such that invalid_locality is computed from the
previous cycle's locality value instead of the newly latched addr[15:12].
This creates a one-cycle window where a VALID stale invalid_locality allows
access to TPM HW registers even when the current transaction's locality
is invalid.

Security Impact
---------------
A host can prime invalid_locality by performing a transaction with a valid
locality, then immediately perform a transaction with an invalid locality.
Because invalid_locality remains based on the prior value for one cycle,
HW gates (and the RegSts path) may authorize access and return real TPM
register contents instead of the expected 0xFF error pattern. This breaks
locality-based access controls and may leak sensitive TPM status or other
HW-managed registers.

Exploit Type
------------
PoC demonstrating unauthorized disclosure (leak) of TPM registers under a
specific back-to-back host transaction waveform. The test coordinates with
the simulation testbench which must drive the SPI host waveform.

Files
-----
- spi_device_locality_vuln_test.c: OTTF test that coordinates with the
  simulation testbench and observes the two back-to-back TPM transactions.
- BUILD: Bazel target to build the OTTF test.

How it works (high level)
-------------------------
1) The test enables the SPI device TPM interface with locality checks and
   return-by-hardware enabled.
2) It emits a SYNC marker and requests the testbench to send two back-to-
   back transactions:
   - A WRITE with addr[15:12] == VALID_LOCALITY (e.g., 0)
   - Immediately followed by a READ with addr[15:12] == INVALID_LOCALITY
     (e.g., 7)
3) The DUT (SPI device) will latch locality and compute invalid_locality
   using the stale value due to non-blocking assignment ordering. The TB
   captures the bytes sent to the host and checks whether the HW returned
   0xFF (expected for invalid locality) or leaked the actual TPM register
   contents.
4) The TB reports pass/fail. The software test logs the observed addresses
   and validates the sequence.

Notes on Implementation
-----------------------
- The test uses DIF APIs for peripheral initialization and interrupt
  handling (acceptable for test initialization). The actual exploit relies
  on hardware waveform timing; therefore the TB must implement the two
  back-to-back SPI transactions precisely as described.
- The software test itself only observes and logs the commands/addresses
  seen by the device. The testbench is responsible for injecting the host
  waveform and for validating the host-observed returned bytes.

Reproducing in Simulation
-------------------------
- Build the test with Bazel::
  bazel build //hw/ip/spi_device:spi_device_vuln_test

- Run the DV simulation environment with the matching testbench harness.
  The test expects the simulation testbench to listen to LOG_INFO sync
  markers and then drive the host SPI waveform:
  * On "SYNC: Begin TPM locality TOCTOU exploit test" the TB should send
    the WRITE with a VALID locality.
  * Immediately after the WRITE header is observed by the device, the TB
    should send the READ with an INVALID locality with no gap between
    transactions.

- The TB should capture the bytes returned to the host during the READ and
  assert whether a leakage (non-0xFF) occurred.

Responsible Disclosure
----------------------
This PoC was generated to reproduce a vulnerability in the OpenTitan
spi_device TPM locality handling. If you are triaging or fixing this bug,
please review the non-blocking assignment ordering in spi_tpm.sv and ensure
invalid_locality is computed from the freshly-latched locality (or by
using blocking assignment or reordering) so that both locality and its
validity are consistent in the same cycle.

References
----------
- File: hw/ip/spi_device/rtl/spi_tpm.sv lines 805-810
- Finding: invalid_locality computed from prior cycle's locality
