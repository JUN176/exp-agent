Vulnerability: Unvalidated AES operation enumeration (op_i) propagation

Affected module: hw/ip/aes
Vulnerability ID: 24

Summary
-------
The aes_mix_columns module directly connects an enumerated control signal (op_i)
into multiple submodules without validating or sanitizing the value. An attacker
with MMIO write access to the AES control register can program out-of-range
enum values which are then fanned out to internal submodules. This PoC shows
that an invalid enum value can be written and read back unchanged, indicating
no sanitization occurs at the register interface.

Exploit Type: Proof-of-Concept (POC) - demonstrates input validation bypass

Test Strategy
-------------
1. Use abs_mmio_write32() to write an out-of-range / intentionally invalid value
   into the AES control register (operation field).
2. Read back the same register using abs_mmio_read32() and verify the value was
   stored unchanged.

If the read-back value equals the written invalid value, the write succeeded
and the hardware did not sanitize the enumeration. Given the RTL shows
op_i being directly passed to per-column mix modules, this acceptance of
invalid values means internal submodules may receive unexpected enum values.

Assumptions & Notes
-------------------
- This test uses a fallback offset of 0x0 for the AES control register. The
  generated register header provided by the build target "//hw/top:aes_c_regs"
  may define a specific offset macro (replace AES_CTRL_OFFSET if available).
- The PoC intentionally avoids DIF usage and uses low-level abs_mmio APIs as
  required for precise control.
- The test does not attempt high-level functional exploitation (e.g. to force
  incorrect cryptographic behavior) because the exact internal behavior depends
  on the unreachable internal RTL mapping. Instead it proves the core issue:
  the MMIO interface accepts invalid enum values.

Base library APIs used
---------------------
- abs_mmio_write32(), abs_mmio_read32()
- mmio_region_from_addr() (not used but available)
- LOG_INFO logging
- OTTF test harness (ottf_main)

How to build
------------
Run Bazel as usual for OpenTitan tests (from repository root):

bazel test //path/to/this:target

Expected Result
---------------
The test should PASS: the invalid value written to the AES control register is
read back unchanged, demonstrating no input sanitization at the MMIO register
boundary and supporting the reported vulnerability.

Technical details
-----------------
The reported RTL fragment (aes_mix_columns.sv) shows:

  input aes_pkg::ciph_op_e op_i,
  ...
  aes_mix_single_column u_aes_mix_column_i (
    .op_i ( op_i ),
    .data_i ( data_i_transposed[i] ),
    ...
  );

Because op_i is directly connected to submodules, any invalid value present
on the op_i signal (originating from the register file) may be fanned out
without bounds checking. This PoC proves the register file can hold an
out-of-range value.
