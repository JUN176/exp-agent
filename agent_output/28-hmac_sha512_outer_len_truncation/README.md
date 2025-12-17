Vulnerability: HMAC outer-SHA-512 message-length miscalculation (Vuln ID 28)

Affected module: hmac

Summary:
The hmac RTL miscomputes the outer-HMAC message length when the digest size is
SHA-512. This bug causes the hardware to under-report the number of bits in
the outer message (using BlockSizeSHA512in64 + 384 instead of +512), resulting
in the SHA engine hashing only the first 384 bits of the inner digest. The
final HMAC-SHA512 tag is thus computed over a truncated inner digest and will
not match the RFC 4231-compliant HMAC-SHA512 value.

Exploit type: POC (detects and proves the non-standard HMAC-SHA512 output)

What this package does:
- Programs the HMAC hardware into HMAC mode with SHA-512 digest size.
- Loads a known 512-bit key and sends the message "Test message." to the
  hardware FIFO (this mirrors an existing reference test vector).
- Triggers processing and reads back the produced HMAC tag.
- Compares the produced tag with the RFC-compliant expected tag. A mismatch
  indicates the RTL bug (truncated outer message) is present.

Base library APIs used:
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32(),
  mmio_region_write8(), mmio_region_nonatomic_set_bit32(),
  mmio_region_get_bit32() (from //sw/device/lib/base:mmio)
- bitfield_field32_write(), bitfield_bit32_write() (from //sw/device/lib/base:bitfield)
- read_32() (from //sw/device/lib/base:memory)
- LOG_INFO() (from //sw/device/lib/runtime:log)

Register headers required (autogen):
- hmac_regs.h (from //hw/top:hmac_c_regs)
- top_earlgrey.h (from //hw/top_earlgrey/sw/autogen:top_earlgrey)

How to build and run:
- bazel test //path/to:hmac_vuln28_poc_test

Expected results:
- If the hardware is standards-compliant, the test will pass (hardware tag ==
  expected RFC HMAC-SHA512 tag).
- If the RTL bug is present (outer message length truncated for SHA-512), the
  test will fail with the message: "HMAC-SHA512 tag mismatch - potential
  outer-message-length bug detected". This demonstrates the security issue
  described in the report.

Notes and limitations:
- The test uses low-level mmio operations to configure the HMAC peripheral and
  push message bytes. It purposely avoids DIF usage for precise control.
- The test assumes the autogen register header names (hmac_regs.h,
  top_earlgrey.h) are present as in OpenTitan build system.
- Timing and sequencing of HASH_START/HASH_PROCESS are implemented to mirror
  the DIF behavior; on some platforms a different ordering may be required.
