Vulnerability: entropy_src Markov health-test outputs disabled (Vuln ID 30)

Summary
- Module: entropy (entropy_src)
- RTL: hw/ip/entropy_src/rtl/entropy_src_markov_ht.sv
- Faulty lines: assign test_fail_hi_pulse_o = 1'b0; assign test_fail_lo_pulse_o = 1'b0; (lines 158-159)

Description
The Markov health test in the entropy_src IP is functionally disabled: the Markov test failure pulse outputs are hardwired to 0, and the module never performs the expected threshold comparisons at window boundaries. This means highly correlated or toggling bitstreams can pass as healthy, undermining entropy quality and monitoring.

Exploit / POC Strategy
1. Configure the entropy source health-test Markov threshold to a very low value so that deterministic/highly correlated input should trigger failures.
2. Enable the firmware-override (FW_OV) interface so firmware can insert controlled data into the entropy pipeline.
3. Stream a deterministic, strongly correlated pattern (e.g., alternating 0xAAAAAAAA / 0x55555555 words) through the FW_OV FIFO to provoke Markov failures.
4. Read back the health-test statistics (high_fails for the Markov test). A functioning Markov test should record failures; the buggy RTL will show zero failures.

What the test does
- Uses DIF to configure entropy_src and health tests, and to stream FW override data.
- Uses DIF to read health-test statistics and checks that Markov high-failure count is non-zero.
- The test intentionally fails (CHECK) if the high-fail counter is zero to demonstrate the bug.

Files
- entropy_src_markov_vuln_test.c: C test using DIF and base APIs to provoke Markov failures.
- BUILD: Bazel rule to build the test.

Build & Run
- bazel test //path/to:entropy_src_markov_vuln_test --test_output=all

Expected Result
- On correct hardware/IP, the Markov health-test will detect the correlated input and increment the high_fails counter for Markov. The test's CHECK(markov_high_fails > 0) will pass.
- On the buggy RTL (with test_fail_* tied low / missing comparisons), the counter will remain zero and the test will fail, proving the vulnerability.

Base library / DIF usage
- Base APIs used: mmio_region_from_addr(), mmio_region_read32(), busy_spin_micros() (via included bases)
- DIF used: dif_entropy_src_* helper functions for configuration, FW_OV FIFO writes, and reading health-test stats. DIF is used to simplify register interactions for the POC; the core demonstration (feeding bad data and reading back stats) relies on low-level control over the entropy source.

Notes
- This POC intentionally fails when the vulnerability is present to make the issue obvious in CI logs.
- Depending on simulation vs silicon, timing and delays (busy_spin_micros) may need adjustment.
