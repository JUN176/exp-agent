Vulnerability: AES KEY_SHARE1 Read Alias to DATA_IN (Vuln ID 1)

Affected Module: aes
RTL: hw/ip/aes/rtl/aes_reg_top.sv
Relevant RTL lines:
  - L0729, L0770, L0811, L0852: nets .qs connected to data_in_[0..3]_qs
  - L1710-L1724: top-level read multiplexer maps addr_hit[9..12] to data_in_[0..3]_qs

Summary:
A read-multiplexer bug in aes_reg_top.sv causes reads to the write-only
KEY_SHARE1_[0..3] CSRs to return the contents of DATA_IN_[0..3]. Although the
individual KEY_SHARE1 subregisters are instantiated with .re(1'b0) (no QS
read path), the top-level reg read multiplexer erroneously selects the
DATA_IN QS nets when KEY_SHARE1 addresses are hit, creating an unintended
alias and breaking the write-only/read-protected semantics.

Security Impact:
This vulnerability allows any agent with CSR read access to extract a full
128-bit plaintext block by writing the plaintext into AES DATA_IN registers
(as part of a regular encryption operation) and then reading back the
KEY_SHARE1_[0..3] addresses. This breaks confidentiality (CWE-212) and can
lead to extraction of plaintext prior to encryption, violating the security
model.

Exploit Type: POC (demonstrates direct plaintext leakage)

Base library APIs used:
 - mmio_region_from_addr, mmio_region_write32, mmio_region_read32 (sw/device/lib/base/mmio)
 - abs_mmio_* (present in BUILD as helper, not strictly required)
 - LOG_INFO (sw/device/lib/runtime/log)
 - CHECK, OTTF test framework (sw/device/lib/testing/test_framework)

Files in this package:
 - aes_keyshare1_read_alias_test.c : C test that writes to DATA_IN and reads KEY_SHARE1
 - BUILD : Bazel test target (opentitan_test) to compile and run the test
 - README.md : This file

How the POC works (step-by-step):
1. Write a chosen 128-bit plaintext (4x32-bit words) into AES DATA_IN_0..3.
2. Immediately read from the addresses corresponding to KEY_SHARE1_0..3.
3. Due to the mux bug, the reads return the values driven by data_in_[0..3]_qs,
   i.e., the plaintext, effectively leaking it via the key-address space.
4. The C test asserts that the read values equal the written plaintext; a
   match indicates the vulnerability is present.

Build & Run:
1. Place this package under the OpenTitan workspace (e.g. under some tests
   directory) and run:
   bazel test //path/to:aes_keyshare1_read_alias_test --test_output=all

Expected Result:
 - If the vulnerability is present, the test will log the leaked words and
   pass the CHECK assertions, returning success.
 - If the hardware/RTL is fixed, the KEY_SHARE1 reads will not match DATA_IN
   and the CHECK will fail, causing the test to fail.

Notes:
 - The test relies on the autogen register header provided by
   //hw/top:aes_c_regs (aes_regs.h) to obtain register offsets such as
   AES_DATA_IN_<n>_REG_OFFSET and AES_KEY_SHARE1_<n>_REG_OFFSET.
 - The test intentionally uses base library mmio APIs to precisely control
   register accesses and avoid higher-level DIF abstractions.
 - No assembly is required for this exploit.
