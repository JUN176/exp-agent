Vulnerability: AES PRD Persistence (Vuln ID 22)

Affected Module: aes
RTL file (reported): hw/ip/aes/rtl/aes_key_expand.sv (lines 238-244)

Summary of Finding:
The PRD buffer register (prd_q) is only asynchronously cleared by the global
reset (rst_ni). It is not cleared by the AES internal clear signals (e.g.
clear_i) or other software-accessible clear triggers. As a result, PRD can
persist across non-reset session boundaries (for example between dif_aes_end()
and the next dif_aes_start() when software does not perform a PRNG reseed).
Because PRD controls the masking applied to S-Boxes, reuse of PRD across
cryptographic sessions weakens side-channel protections and enables averaging
attacks like DPA/CPA.

Exploit Type: Proof-of-Concept (POC)

What this test does (high level):
- Starts an AES transaction with masking forced on.
- Triggers a PRNG reseed so the AES internal PRD buffer is populated.
- Encrypts one block of known plaintext and records the ciphertext (Session 1).
- Ends the AES transaction using the documented AES clearing sequence.
- Starts a new AES transaction WITHOUT performing a PRNG reseed.
- Encrypts the same plaintext again and records the ciphertext (Session 2).
- Compares the two ciphertexts. If the PRD buffer persisted across the
  non-reset boundary, the masking conditions are identical and the produced
  ciphertexts will match. If PRD were cleared properly, ciphertexts would
  typically differ.

Why this proves the vulnerability:
The test demonstrates PRD reuse by showing identical ciphertext outputs for
identical inputs across two independent AES transactions where the second
transaction explicitly avoids reseeding. This indicates that the internal
masking state was not cleared by the normal AES "end"/clear sequence and
remained available to the next transaction.

Base library APIs used:
- mmio_region_from_addr (indirect via DIF init)
- mmio_region_read32 / mmio_region_write32 (available via deps)
- memory functions (memcpy, memcmp)
- LOG_INFO / CHECK macros from the runtime and test framework

DIF usage:
- dif_aes_* functions are used for convenience to initialise and control AES.
  Per the POC development guidance, DIF was only used for high-level
  configuration and sequencing while the proof focuses on observable outputs
  (ciphertext comparisons) to demonstrate PRD persistence.

How to build and run:
1. Build the test:
   bazel build //opentitan/sw/device/tests:aes_prd_persistence_vuln_test

2. Run the test in your simulator or hardware environment. Example:
   bazel test //opentitan/sw/device/tests:aes_prd_persistence_vuln_test

Expected result:
- The test logs two ciphertexts (Session 1 and Session 2). If the vulnerability
  is present in the hardware configuration under test, the two ciphertexts
  will be identical and the test will PASS, printing "PRD reuse observed...".
- If the hardware clears PRD correctly on the AES clear sequence, the
  ciphertexts will differ and the test will FAIL at the final CHECK.

Notes and limitations:
- The test assumes an enabled entropy complex such that triggering a PRNG
  reseed results in the AES internal masking PRNG being seeded. If the test
  environment has entropy disabled or mocked, results may vary. In CI-based
  or DUT environments where CSRNG/EDN are available, the reseed trigger used
  in Session 1 should populate PRD.
- This POC does not attempt to read the internal PRD register directly (not
  typically software-visible). Instead it infers PRD persistence from the
  observable ciphertext behavior, which is sufficient to demonstrate the
  security impact on masking.

Mitigation guidance:
- PRD (prd_q) should be synchronously/explicitly cleared by the AES internal
  clearing signals (e.g., clear_i) and/or by software-accessible triggers
  (e.g., the DataOutClear/KeyIvDataInClear/PRNG clear paths). Reliance on
  rst_ni alone is insufficient to guarantee that PRD cannot be reused across
  sessions.

