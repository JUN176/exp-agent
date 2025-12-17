Vulnerability: Default OTP key response tie-off encoding (OTP_CTRL)
Vulnerability ID: 34

Summary:
The SystemVerilog parameter FLASH_OTP_KEY_RSP_DEFAULT in otp_ctrl_pkg.sv uses obfuscated constant expressions that evaluate to a response indicating acknowledgement and seed validity while the key material is all zeros. Downstream state machines that trust these flags may proceed with a zero key, disabling or weakening flash scrambling and exposing flash contents.

What this package contains:
- otp_ctrl_vuln_test.c : A C-based POC that reconstructs and evaluates the SystemVerilog expressions from otp_ctrl_pkg.sv to demonstrate that the default response encodes ack/seed_valid as true while the key and rand_key are all zeros.
- BUILD : Bazel test rule to build the POC (uses base libraries only).

Exploit type: POC / Proof-of-Concept
This test does NOT attempt to manipulate hardware registers. Instead, it proves at software level that the RTL expressions evaluate to the dangerous default values. This is sufficient to demonstrate the specification/implementation mistake and to guide a hardware-level test where an OTP failure or tie-off could cause the default to be observed by consumers.

Base library APIs used:
- None required for the logical evaluation portion. The test includes base headers in case of extensions.

How to build and run:
1. Place this folder under the OpenTitan workspace.
2. Build with Bazel: bazel build //path/to/otp_ctrl_vuln_test
3. Run the test in simulation: bazel test //path/to/otp_ctrl_vuln_test --test_output=all

Expected result:
- The test will log the computed intermediate values and assert that data_ack==1, seed_valid==1, and the key words are all zero. All CHECK assertions should pass, indicating the RTL literal expressions are consistent with the reported vulnerability.

Technical details / rationale:
- The key fields are explicitly constructed as XORs of zero vectors and '0 literals, guaranteeing zero-valued keys.
- The ack and seed_valid fields are constructed from reductions and logical operations that evaluate to 1 for constant inputs (zeros and ones), creating a scenario where a 'valid' flag is asserted despite absent key material.
- Downstream consumers that only check seed_valid and ack bits before using the key can be induced to accept zero keys. In silicon this could be triggered by error conditions, tie-off nets, or mis-integration.

Notes for follow-up hardware POCs:
- A hardware-level proof would perform a sequence that causes the OTP response path to be unavailable (e.g., force ECC uncorrectable error or toggle prim_otp responses) and then read the consumer registers which sample FLASH_OTP_KEY_RSP_DEFAULT. The consumer should be observed accepting the response and proceeding with zero key material.

References:
- RTL excerpt (otp_ctrl_pkg.sv lines 157-163) analyzed in this POC.

