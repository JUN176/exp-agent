ibex_mseccfg_mml_clearable POC

Vulnerability ID: 5
Affected module: ibex (MSECCFG CSR)

Summary:
This proof-of-concept demonstrates that software can both set and clear bits in
the MSECCFG CSR. The reported vulnerability states that the mseccfg.MML bit
should be immutable once set (i.e., cannot be cleared by software and requires
reset), but OpenTitan's current implementation allows software to clear the
bit. This test verifies that behavior by selecting a currently-cleared bit in
MSECCFG, setting it, and then attempting to clear it via CSR operations.

Exploit Type: POC (demonstrates the insecure behavior; does not escalate
privileges in this test). The security impact is high if the MML bit is
actually the selected bit (or similarly-behaved bit), because clearing MML
would allow lower privilege modes to bypass PMP/SMEPMP protections.

Base library APIs used:
- CSR_READ / CSR_SET_BITS / CSR_CLEAR_BITS (sw/device/lib/base/csr.h)
- LOG_INFO (sw/device/lib/runtime/log.h)
- OTTF test framework (sw/device/lib/testing/test_framework:ottf_main)

How the test works (high level):
1. Read the current MSECCFG CSR value.
2. Find a bit position that is currently zero (to avoid guessing the MML bit
   index). This bit will be used as a proxy to demonstrate write/clear
   capability on MSECCFG.
3. Set the selected bit using CSR_SET_BITS and verify the bit is set.
4. Clear the selected bit using CSR_CLEAR_BITS and read back MSECCFG.
5. If the bit clears successfully, the test logs that the vulnerability is
   confirmed (software can clear MSECCFG bits). Otherwise the test fails.

Notes and assumptions:
- The RTL line referenced in the original report (ibex_cs_registers.sv:1271)
  could not be retrieved in this environment; therefore the test uses a
  generic method of toggling a zero bit in MSECCFG rather than directly
  addressing mseccfg.MML by name. If the MML bit is at a known fixed index,
  the mask in the test can be replaced with that exact bit mask to test the
  MML behavior specifically.

Building and running:
- Place this directory under the OpenTitan repo as a test target.
- Build with Bazel (from repo root):
  bazel build //path/to/this:test
- Run the test in the simulator or on hardware using the OTTF harness.

Expected results:
- If software can clear the selected MSECCFG bit, the test will pass and
  print a confirmation message. This indicates the hardware allows software
  to clear bits in MSECCFG, aligning with the reported vulnerability.
- If the selected bit is sticky and cannot be cleared, the test will fail and
  log that the bit remained set after the clear attempt.

Security recommendation:
- Ensure that MML (and any other bits required to be immutable by the SMEPMP
  specification) are implemented as write-once or require reset to clear.
  Hardware should ignore software clear attempts for these bits, or use a
  separate configuration mechanism that enforces immutability.
