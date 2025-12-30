Vulnerability POC: HMAC err_code missing invalid_config_atstart

Vulnerability ID: 38
Module: hmac
Affected RTL: hw/ip/hmac/rtl/hmac.sv (lines ~842-858)

Summary:
The RTL computes err_valid using a set of error conditions that includes
invalid_config_atstart. However, when assigning err_code in a priority case,
invalid_config_atstart is not handled (and there is a duplicated case entry
for hash_start_sha_disabled), leaving err_code at its default value
NoError while the hmac_err interrupt can be asserted. This leads to an
interrupt with ERR_CODE == NoError, masking the root cause for firmware.

Exploit Type: Proof-of-Concept (information/diagnostic masking)

What this POC does:
- Enables the HMAC error interrupt (HMAC_ERR).
- Writes an invalid configuration (digest_size = SHA2_None) which makes
  `invalid_config` true in the RTL.
- Issues a HASH_START command which sets reg_hash_start simultaneously with
  invalid_config, triggering invalid_config_atstart.
- Verifies that the HMAC_ERR interrupt bit in INTR_STATE is set while
  ERR_CODE register contains NoError (0).

Base library APIs used:
- abs_mmio_write32 / abs_mmio_read32
- mmio_region_from_addr / mmio_region_read32
- bitfield_field32_write / bitfield_bit32_write / bitfield_bit32_read
- LOG_INFO, CHECK, OTTF test framework

How to build and run:
1. bazel build //path/to:hmac_vuln_test
2. bazel test //path/to:hmac_vuln_test --test_output=all

Expected result:
- The test should pass, demonstrating the condition where an HMAC error
  interrupt is raised but ERR_CODE == NoError. The log will show INTR_STATE
  and ERR_CODE values and a final message "POC successful".

Notes and technical details:
- The RTL lines show err_valid includes invalid_config_atstart but the
  priority case that sets err_code has no corresponding entry for
  invalid_config_atstart. Consequently, when invalid_config_atstart is the
  first event, err_code remains NoError.
- This allows an attacker to cause an interrupt to be raised without a
  meaningful ERR_CODE; because further errors are blocked while an
  interrupt is pending, this can be used to mask more important errors until
  firmware clears the interrupt.
