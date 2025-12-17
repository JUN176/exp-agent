Vulnerability: ibex instruction cache forced on in debug mode
Module: ibex
Vulnerability ID: 16

Summary:
The RTL for the ibex core forces the instruction cache (icache) to be enabled
whenever the processor is placed into debug mode, overriding the software-
controlled icache_enable bit in the CPUCTRL CSR. This POC demonstrates how to
detect that behavior from software using only base library APIs.

Exploit Type: POC / Verification
This test only verifies the existence of the behavior (icache forced on in
debug mode). It does not perform automated cache staleness exploitation.

What the test does (high level):
1) Reads the CPUCTRL CSR and attempts to clear the software-controllable icache
   enable bit.
2) Simulates entering debug mode by setting a presumed debug indicator bit in
   the DCSR CSR.
3) Reads back the CPUCTRL CSR and checks whether the icache enable bit has
   been forced to 1 by the hardware despite the earlier software clear.

Base library APIs used:
- CSR_READ / CSR_SET_BITS / CSR_CLEAR_BITS (csr.h)
- LOG_INFO (runtime/log.h)
- CHECK (testing/test_framework/check.h)
- Standard C memory helpers (memory.h) - included for completeness

Files in this package:
- ibex_icache_dbg_vuln_test.c : C test implementing the POC
- BUILD : Bazel build rule to compile the test
- README.md : This document

Assumptions and Notes:
- The RTL file specified in the vulnerability report could not be read by the
  agent. The POC is therefore written using the published CSR interface and
  the vulnerability description. The exact bit positions of the icache enable
  field and the debug indicator in DCSR are implementation-dependent.

- The test uses the following assumed masks (adjust if the DUT uses different
  encodings):
  * CPUCTRL_ICACHE_ENABLE_MASK: (1u << 0)
  * DCSR_DEBUG_MASK: (1u << 0)
  If the DUT uses different masks, update these definitions in
  ibex_icache_dbg_vuln_test.c to the correct bit positions from the RTL.

- On many RISC-V systems entering debug mode is performed by an external
  debugger and may not be achievable by software writes to DCSR on the same
  hart. If setting DCSR from software does not place the core into the same
  debug state used by the RTL forcing logic, reproduce the test by using an
  external debugger or by running in a simulator where the debug state can be
  asserted.

How to run:
1) Place the test under the OpenTitan workspace and build with Bazel:
   bazel build //path/to:ibex_icache_dbg_vuln_test

2) Run the test in the appropriate simulation environment. See the BUILD file
   exec_env settings for supported environments.

Expected result:
- On a device exhibiting the documented vulnerability the test will observe
  that the icache enable bit in CPUCTRL is set while the core is in debug mode
  even after software cleared it, and the test will log "VULN TRIGGERED".

- If the device does NOT exhibit the vulnerability the test fails with the
  check "icache was not forced on in debug mode".

Technical details:
- Forcing the icache on in debug mode can allow stale instructions to execute
  after instruction memory is modified while the core is halted. This POC
  verifies the hardware-level override; a follow-up exploit could attempt to
  demonstrate stale instruction execution by halting the core in debug, having
  a secondary agent modify instruction memory, then resuming execution and
  showing that the core executes old instructions from the cache.

Contact:
- For reproducing or patching guidance, consult the ibex maintainers and the
  OpenTitan security team.
