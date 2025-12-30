Vulnerability: HMAC unmapped read leaks key word (Vulnerability ID 29)

Affected Module: hmac

Summary:
This POC demonstrates the vulnerability described in report V29 where an unmapped
read (decode miss) from the HMAC peripheral can return reg2hw.key[2].q (a 32-bit
word of the secret key) due to a default case in the register read multiplexer.
When the address decoder does not assert any addr_hit signals, the default arm
drives the read-data bus with a secret key word, leaking it onto the TL-UL bus.

Exploit Type: Proof-of-Concept (information exposure)

What this package does:
- Uses the DIF HMAC helper (only for initialization) to load a known 256-bit key
  into the HMAC internal registers.
- Probes a broad range of offsets within the HMAC peripheral address window
  using low-level absolute MMIO reads (abs_mmio_read32) to search for any
  reads that return the expected key word (word index 2).
- Reports and asserts if any read returns the expected key word, proving
  the leakage.

Why DIF is used:
The DIF is used solely to load a known key into the HMAC internal registers so
that leaked values are predictable and easily identified. The actual leakage
check and reads are performed using base library MMIO functions (abs_mmio_*)
per the POC development policy.

Base library APIs used:
- abs_mmio_read32()
- LOG_INFO()
- CHECK() assertions
- memcpy() from memory.h

How to build and run:
- Add this test to the bazel workspace and run:
    bazel test //path/to:hmac_v29_poc_test

- It is recommended to run in simulation (sim_verilator or sim_dv) where the
  internal behavior can be observed deterministically.

Expected results:
- The test will log offsets returning the expected key word (0x33333333 in the
  provided key pattern). If any such offsets exist within the probed range,
  the test will pass and report the number of matches. If no matches are found
  the test will fail (indicating no leakage observed in this configuration).

Notes and caveats:
- The test probes a heuristic address range (0x0 -- 0x400). Register maps may
  vary across IP versions; if no matches are found, consider expanding the
  probed range for the specific hardware model under test.
- Some matches correspond to legitimate key/digest registers; the test logs all
  matches so the operator can inspect whether the matching offsets are
  documented registers or unexpected/unmapped addresses.
- This POC intentionally uses low-level MMIO accesses to exercise the TL-UL
  read-data path directly, which is the locus of the reported vulnerability.
