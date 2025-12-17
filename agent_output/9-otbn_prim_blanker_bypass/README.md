Vulnerability: OTBN prim_blanker enable hardcoded -> DMEM/IMEM readable during OTBN operations

Module: otbn
Vuln ID: 9

Summary:
The prim_blanker modules intended to prevent reads of IMEM/DMEM during OTBN operations are misconfigured (enable signals hardcoded), allowing an attacker with system bus MMIO access to read OTBN memory while OTBN is busy. This POC demonstrates reading DMEM during a secure-wipe operation (OTBN busy) which should have been protected.

Exploit Type: POC (demonstrates data leakage from OTBN DMEM during busy operation)

What the test does (high level):
- Writes a known secret pattern into OTBN DMEM via the MMIO DMEM window.
- Triggers a secure wipe of DMEM (OTBN CMD = 0xC3) which places OTBN into a busy wipe state.
- While OTBN reports busy wiping, the test repeatedly attempts to read DMEM via the system bus MMIO window.
- If the read returns the secret pattern while OTBN is busy, the test reports that the vulnerability is present.

Base library APIs used:
- mmio_region_from_addr(), mmio_region_memcpy_to_mmio32(), mmio_region_memcpy_from_mmio32(),
  mmio_region_write32(), mmio_region_read32() from //sw/device/lib/base:mmio
- memory helpers from //sw/device/lib/base:memory
- LOG_INFO, CHECK from test framework/runtime

Why this proves the vulnerability:
OTBN is performing an internal secure-wipe operation and is expected to prevent bus-side reads of DMEM. Successfully reading back the secret pattern while OTBN reports it is busy shows that the bus-side blanking protection is not effective (as the prim_blanker modules are misconfigured).

How to build and run:
- Place the test under an appropriate test package and run via Bazel as an opentitan_test target.
- Example build invocation:
    bazel test //path/to:otbn_vuln_test --test_output=all

Expected result:
- The test passes when it observes that DMEM can be read while OTBN is busy wiping DMEM. The test will fail if the read is blocked (i.e., no leak observed).

Notes and caveats:
- This POC uses low-level base MMIO APIs rather than DIF to precisely control timing and memory operations.
- The test triggers OTBN secure-wipe to create a long-enough busy window; other long-running OTBN operations could be used (e.g., a long-running app) if available.
- The exact path of autogen headers is provided via BUILD deps; ensure Bazel workspace is configured (the target depends on //hw/top:otbn_c_regs and //hw/top_earlgrey/sw/autogen:top_earlgrey).

References:
- RTL: hw/ip/otbn/rtl/otbn.sv (line 690 shows dmem_rdata_bus_en_d is assigned constant 1'b1)

