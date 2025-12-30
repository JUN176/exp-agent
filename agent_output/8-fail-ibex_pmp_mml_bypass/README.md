Vulnerability: IBEX PMP M-mode bypass when MML==0 (Vuln ID 8)

Affected module: ibex (PMP handling)

Summary:
This POC demonstrates that when a PMP region is configured with the lock bit
(L) set and no permissions (R/W/X cleared), a load from that region executed
in Machine mode (M-mode) should raise a load access fault. The vulnerability
report indicates that when the MSECCFG.MML bit is cleared (MML == 0), the CPU
incorrectly allows M-mode access to PMP-protected regions and no exception is
raised, violating SMEPMP security rules.

Exploit type: Proof-of-Concept (demonstrates incorrect behavior leading to
PMP protection bypass for M-mode).

What this package contains:
- ibex_pmp_vuln_test.c : C test using low-level CSR access to configure PMP
  and trigger a load from a locked PMP region. Uses base library CSR macros
  (CSR_READ/CSR_WRITE) and OTTF test framework.
- BUILD : Bazel test rule to build the test.

Base library APIs used:
- CSR_READ / CSR_WRITE (sw/device/lib/base/csr.h)
- LOG_INFO (sw/device/lib/runtime/log.h)
- OTTF test framework (sw/device/lib/testing/test_framework)
- CHECK macro (sw/device/lib/testing/test_framework/check.h)

Why this demonstrates the vulnerability:
1. The test creates a 4-byte aligned word in memory.
2. It writes PMPADDR7 = target_addr >> 2 and sets the PMPCFG1 byte for region
   7 to L=1 and A=NA4 (4-byte region) with no R/W/X permissions.
3. It clears MSECCFG (writing 0) to ensure MML==0 as described in the report.
4. It performs a load from the protected address while running in M-mode.

Expected correct behavior (per spec): the load should trap and the test's
custom load-store fault handler should be invoked (pmp_load_exception==true).

Observed (vulnerable HW): the load does not trap and pmp_load_exception stays
false. The CHECK will fail, indicating the PMP protection was bypassed in
Machine mode.

How to run:
1. Place this package in the repository under a tests directory.
2. Build and run via bazel (example):
   bazel test //path/to:ibex_pmp_vuln_test --test_output=all

Notes and limitations:
- This POC directly writes CSR registers and relies on NATURAL defaults and
  available PMP region (region 7). If the target environment uses region 7
  for ROM/ROM_EXT, pick a different region and adjust indices accordingly.
- The test writes MSECCFG = 0 to force MML==0. The bit layout of MSECCFG is
  implementation-defined; clearing the register is the simplest way to force
  MML==0 on devices that follow the vulnerability description.
- No assembly is required for this POC since CSR helpers and direct memory
  access suffice.

References:
- RISC-V privileged specification: Physical Memory Protection (PMP) and pmpcfg

