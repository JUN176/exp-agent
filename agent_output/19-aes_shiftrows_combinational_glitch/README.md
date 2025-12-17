AES ShiftRows Combinational Glitch POC

Vulnerability ID: 19
Affected module: hw/ip/aes (aes_shift_rows.sv)

Summary
- The AES ShiftRows module drives its outputs combinationally from data_i and op_i.
- Rows 1 and 3 use variable-shift mux trees controlled by row1_shift/row3_shift, which are derived from op_i in an always_comb block.
- If op_i is changed asynchronously or data_i and op_i are not stable in the same clock domain, mux hazards and path imbalance can produce transient glitches on data_o. These glitches can leak side-channel information or create exploitable faults.

Exploit Type
- Proof-of-Concept: demonstrates that rapid toggling of the AES operation control (op_i) during data setup can produce different ciphertext outputs compared to a clean run, indicating combinational hazards.

POC Approach
- Use low-level base library APIs (mmio_region_*) to directly write AES registers.
- Perform two AES encryptions with identical keys and plaintexts:
  1) Clean run: normal manual-mode encryption without toggles.
  2) Toggled run: write identical configuration but rapidly toggle the operation field (encrypt/decrypt) in the control shadow register right after loading data and before triggering START.
- Compare outputs. A difference indicates the presence of combinational glitches/faults caused by mid-cycle changes to op_i.

Files in package
- BUILD: Bazel rule to build the test (opentitan_test named aes_shiftrows_vuln_test).
- aes_shiftrows_vuln_test.c: C test implementing the POC using base APIs.

Base library APIs used
- mmio_region_from_addr, mmio_region_write32, mmio_region_read32, mmio_region_get_bit32
- bitfield_field32_write, bitfield_bit32_write
- LOG_INFO, CHECK

Notes and Limitations
- This POC attempts to maximize the chance of triggering combinational hazards by performing many back-to-back control register writes without waiting for internal AES state transitions. Whether a glitch is observed can be timing- and simulation/fabric-dependent.
- On real silicon, such hazards are exploitable as side-channel leakage amplification or differential faults under CDC misuse or metastability conditions. In simulation the effect may be easier or harder to observe depending on timing models.

How to build and run
- bazel build //path/to:aes_shiftrows_vuln_test
- bazel test //path/to:aes_shiftrows_vuln_test --test_output=all

Expected results
- The test logs the clean and toggled outputs. If they differ, the test passes (returns success) and logs that a vulnerability was observed.
- If outputs are identical, the POC did not observe a glitch (test fails).

Mitigation
- Register the op_i and/or data_i boundaries so that ShiftRows sees stable, synchronous inputs (insert flop boundary), or make the shift selection registered to avoid combinational mux hazards.

References
- hw/ip/aes/rtl/aes_shift_rows.sv lines 32-35

