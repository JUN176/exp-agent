ibex PMP access_fail bypass (Vulnerability ID 7)

Summary:
This POC demonstrates a critical bug in the ibex PMP implementation where the
internal signal `access_fail` is hardcoded to 1'b0 (see ibex_pmp.sv line 139).
This effectively disables PMP checks that should enforce Machine Mode Whitelist
Policy (MMWP), privilege validation, and Machine Mode Lock (MML) execution
protections. The result is that U-mode code may access regions that should be
denied by PMP, enabling arbitrary reads/writes and potential privilege
escalation.

Exploit Type: POC (demonstrates bypass of PMP protections leading to U-mode
write succeeding against a PMP-protected region).

What the test does:
- In M-mode, the test allocates a small target variable in memory and records
  its address.
- It programs a PMP NAPOT entry (pmp0) that covers a 4 KiB region aligned to
  that address and sets R/W/X = 0 (deny all) for the region.
- It installs a trap handler (M-mode) to catch exceptions from U-mode.
- It clears MSTATUS.MPP to U-mode and uses a small assembly helper to set
  mepc to a U-mode function that attempts to write to the protected address
  and then performs an ECALL.
- If the write is blocked by PMP, a load/store access fault occurs and the
  trap handler records failure. If the write succeeds (PMP bypassed), the
  U-mode code performs an ECALL; the trap handler records success.
- The test reports a vulnerability presence when the U-mode write succeeds.

Base library APIs used:
- CSR_READ / CSR_WRITE (sw/device/lib/base/csr.h)
- LOG_INFO (sw/device/lib/runtime/log)
- OTTF test framework (test harness)

Assembly components:
- ibex_pmp_vuln.S contains:
  - enter_user_mode (sets mepc and mret to switch to U-mode)
  - user_func (runs in U-mode, performs the write and ecall)
  - trap_handler (M-mode trap handler that distinguishes ECALL vs access
    fault and returns to M-mode at a safe point)

How to build and run:
- Place the files under an appropriate test directory in the OpenTitan tree.
- Build with Bazel:
  bazel build //path/to:ibex_pmp_vuln_test
- Run in simulation or on hardware (if supported).

Expected results:
- On a vulnerable ibex core (with access_fail hardcoded), the U-mode write
  will succeed despite PMP denying the region, and the test will report the
  vulnerability (returns success from the POC to indicate exploit worked).
- On a correct implementation, the U-mode access will trap and the test will
  fail the CHECK assertion (indicating PMP blocked the access as expected).

Notes & caveats:
- The test manipulates machine-mode CSRs and installs a trap handler; it must
  be run in an environment where user-mode transitions and exception handling
  are permitted (simulation or real hardware on the target platform).
- The exact CSR names and bitfields used rely on the platform's csr.h and
  csr_registers.h definitions present in the OpenTitan tree.
- This POC intentionally uses low-level base library CSRs and assembly to
  precisely control privilege transitions and exception handling.
