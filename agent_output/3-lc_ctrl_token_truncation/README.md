# LC_CTRL Token Truncation Vulnerability (Vulnerability #3)

## Affected Module
Lifecycle Controller (lc_ctrl)

## Vulnerability Description
The vulnerability stems from incomplete cryptographic hash validation in the Lifecycle Controller FSM. Specifically, the code compares only the lower 32 bits of two 128-bit hashed tokens (hashed_token_i and hashed_token_mux), despite security annotations (SEC_CM: TOKEN.DIGEST) and design documents explicitly requiring full 128-bit cSHAKE hash validation. This partial comparison reduces the effective security strength from 2^128 to 2^32.

## Exploit Type
Proof of Concept (POC)

## Base Library APIs Used
- `abs_mmio_read32()` - For reading LC_CTRL registers
- `abs_mmio_write32()` - For writing to LC_CTRL registers
- `LOG_INFO()` - For logging information during the test

## How It Works
This POC demonstrates that the LC_CTRL FSM only validates the lower 32 bits of 128-bit tokens rather than the full 128 bits as required. By writing a token where only the lower 32 bits match the expected value (and the upper 96 bits can be arbitrary), we can show that the vulnerability exists.

In a real-world scenario, this would allow an attacker to:
1. Reduce the complexity of brute-forcing a valid token from 2^128 to 2^32 attempts
2. Bypass critical security barriers with a 2^96 reduction in brute-force complexity
3. Enable unauthorized lifecycle state transitions (e.g., to FlashRmaSt)
4. Potentially lead to RMA mode abuse for sensitive data extraction

## Expected Results
The test will demonstrate that only the lower 32 bits of the token are validated by:
1. Writing a 128-bit token with arbitrary upper 96 bits
2. Showing that a transition attempt can proceed with only the lower 32 bits matching
3. Logging information about the vulnerability and its security implications

## Build and Run
```bash
# Build the test
bazel build //path/to/test:lc_ctrl_token_truncation_test

# Run the test in simulation
bazel run //path/to/test:lc_ctrl_token_truncation_test_sim_dv
```

## Technical Details
The vulnerability is in the LC_CTRL FSM implementation where token validation is performed. Instead of comparing all 128 bits of the hashed tokens, only the lower 32 bits are checked. This implementation flaw significantly weakens the security of the lifecycle controller's token-based state transitions.