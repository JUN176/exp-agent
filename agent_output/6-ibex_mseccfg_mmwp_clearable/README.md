Vulnerability POC: ibex MSECCFG.MMWP Clearable by Software

Vulnerability ID: 6
Module: ibex
RTL location (reported): hw/vendor/lowrisc_ibex/rtl/ibex_cs_registers.sv:L1272

Summary:
The MSECCFG CSR contains the MMWP bit that, according to the SMEPMP
specification, must be irreversible once set (i.e., it can only be cleared
by a system reset). The OpenTitan ibex implementation exposes the MSECCFG
CSR to software and allows clearing the MMWP bit at runtime. This POC
demonstrates that software can set and then clear the MMWP bit using CSR
write/clear operations, which constitutes a deviation from the SMEPMP
security model.

Exploit type: POC (demonstrates the presence of the vulnerability by
clearing the MMWP bit in software; does not by itself perform privilege
escalation but shows a fundamental violation enabling further attacks).

Test strategy:
1. Read the current MSECCFG CSR.
2. Set the MMWP bit using CSR_SET_BITS(CSR_REG_MSECCFG, mask).
3. Verify the bit is set.
4. Attempt to clear the bit using CSR_CLEAR_BITS(CSR_REG_MSECCFG, mask).
5. Read back the CSR. If the bit is cleared, the test passes (vulnerable).

Important implementation notes:
- The RTL uses a symbolic index CSR_MSECCFG_MMWP_BIT. For portability this
  POC assumes the MMWP bit is bit 0 (mask = 1u << 0). If a particular
  build defines the bit at a different index, adjust mmwp_mask in the C
  test accordingly to match the device under test.
- This POC uses base library CSR macros (CSR_READ, CSR_SET_BITS,
  CSR_CLEAR_BITS) to directly exercise the CSR read/write semantics. No
  DIF APIs are used.

Files in this package:
- ibex_mseccfg_vuln_test.c  : C test implementing the POC (uses base CSR
                               macros and OTTF test framework)
- BUILD                    : Bazel rule to build the test
- README.md                : This file

How to build and run:
1. bazel build //path/to/this/dir:ibex_mseccfg_vuln_test
2. bazel test //path/to/this/dir:ibex_mseccfg_vuln_test

Expected results:
- Vulnerable device: The test will pass, because the MMWP bit can be
  cleared by software and the final CSR read will show the bit cleared.
- Secure device (spec-compliant): The test will fail at the final CHECK,
  because the MMWP bit remains set even after CSR_CLEAR_BITS; this is the
  expected secure behavior.

Base library APIs used:
- CSR_READ / CSR_SET_BITS / CSR_CLEAR_BITS (sw/device/lib/base/csr.h)
- LOG_INFO (sw/device/lib/runtime/log.h)
- OTTF test framework (sw/device/lib/testing/test_framework:ottf_main)

Notes for reviewers:
- If the MMWP bit index is different than bit 0 on your target, update
  mmwp_mask in the C source before running the test.
- This POC intentionally asserts that the bit becomes clear; the assertion
  passing indicates a security violation.
