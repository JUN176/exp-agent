Vulnerability: pfifo_postht_clr TOCTOU (entropy_src) - ID 26

Affected module: entropy_src (post-health-test packer FIFO clear logic)

Summary:
The RTL at entropy_src_core.sv:L2499-L2501 computes the clear signal for the
post-health-test packer FIFO (pfifo_postht_clr) as:
  pfifo_postht_clr = fw_ov_mode_entropy_insert ? (~es_enable_fo[7] & pfifo_postht_not_empty) : ~es_enable_fo[7];
Because pfifo_postht_not_empty is a registered version of the FIFO valid
signal, it can lag the acceptance of a write by one cycle. When the design
has no clear-wins guarantee or push masking on disable, a last-cycle push
concurrent with es_enable_fo[7] falling can be accepted while the clear is
suppressed. This leaves stale post-HT entropy resident across disable->enable
boundaries and violates flush-on-disable semantics (TOCTOU).

Exploit Type: Proof-of-Concept (POC)
This POC attempts to trigger the race by performing a tight sequence of
operations from firmware in FW-override entropy-insert mode:
  1. Enable FW-override mode with entropy insert enabled.
  2. Push a known magic word into the FW-overridden data port.
  3. Immediately toggle the entropy-insert enable (disable then re-enable)
     to create a disable->enable boundary.
  4. Read the observation FIFO and check whether the magic word persists.

If the magic word is observed after the disable->enable cycle, the issue is
present: the post-HT FIFO was not guaranteed to be cleared and stale data
survived the transition.

Base library APIs used:
  - mmio_region_from_addr, mmio_region_read32, mmio_region_write32 (mmio)
  - abs_mmio_* (available if needed)
  - bitfield_field32_write (bitfield)
  - LOG_INFO (runtime/log)
  - CHECK (test framework check.h)

Assembly: None required.

How to build and run:
  1. Add this test to your workspace and ensure the BUILD dependencies are
     satisfied (the generated entropy_src register header must be available
     via //hw/top:entropy_src_c_regs).
  2. Build and run in simulation (the race is timing-dependent; multiple
     runs may be needed):
     bazel test //path/to:entropy_src_pfifo_postht_vuln_test --test_output=all

Expected results:
  - Vulnerable hardware: The test will detect the magic word after the
    disable->enable boundary and assert (CHECK failure), demonstrating the
    TOCTOU condition and presence of stale data.
  - Fixed hardware or unlucky timing: The test will not find the magic word
    (no CHECK failure). This does not prove absence of the issue in general
    due to timing sensitivity.

Notes and limitations:
  - This POC relies on tight software timing to create a last-cycle write vs
    disable window. In many simulation environments additional stimulus or
    a constrained clock may be needed to consistently reproduce the race.
  - The test intentionally uses base MMIO APIs for precise control and to
    avoid DIF-level abstractions.
  - If reproducing locally, consider running the test many times or adding
    small hardware-friendly delays (e.g. NOP loops or CSR reads) to increase
    likelihood of hitting the race window.

References:
  - RTL lines: hw/ip/entropy_src/rtl/entropy_src_core.sv:2498-2501
  - CWE-666: Operation on Resource in Wrong Phase/TOCTOU
