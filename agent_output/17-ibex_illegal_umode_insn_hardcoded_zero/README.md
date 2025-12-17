Vulnerability: ibex illegal_umode_insn hardcoded to 0 (Vuln ID: 17)

Affected module: ibex (ID stage)

Overview:
The signal illegal_umode_insn, intended to flag instructions that are illegal when
executed in User Mode, was reported to be hardcoded to a constant 0. This removes
an additional defense-in-depth check: if a new privileged instruction is added
elsewhere without a corresponding privilege check, this disabled signal would
prevent detection at this stage.

Exploit / POC strategy:
- Type: Proof-of-Concept (detects absence of redundant illegal-in-U-mode trap)
- Idea: Transition the core to User Mode and execute a privileged instruction.
  If the CPU correctly enforces privilege, executing a privileged instruction
  (mret) in U-mode should raise an illegal instruction exception (mcause=2).
  If the redundant illegal_umode_insn check is disabled and other protections
  are absent for a hypothetical new privileged opcode, the instruction could
  execute without trapping. This test detects that condition.

What the test does:
1. Places the 32-bit instruction encoding for `mret` (0x30200073) into a
   small writable buffer in memory.
2. From M-mode, writes the address of that buffer to MEPC and clears the MPP
   bits in MSTATUS (setting MPP=User).
3. Executes `mret` from M-mode, which transfers execution to the buffer in
   User Mode. The first instruction executed there is `mret` (privileged).
4. After the trap (or if no trap occurs), the test reads MCAUSE.
   - Expected: MCAUSE == 2 (illegal instruction)
   - If not: the test fails and reports a potential vulnerability.

Base library APIs used:
- CSR_READ / CSR_WRITE (sw/device/lib/base/csr.h)
- LOG_INFO (sw/device/lib/runtime/log.h)
- OTTF test framework (sw/device/lib/testing/test_framework)

Why this demonstrates the issue:
- The test forces the exact scenario the disabled signal was meant to cover: a
  privileged instruction executing in User Mode. If the additional check is
  permanently disabled in the ID stage, this redundant protection is missing
  and the test will detect the absence of the expected trap.

Build & run:
- Place the provided files under a test package and run via Bazel:
  bazel test //path/to:ibex_vuln_test

Expected results:
- Correct hardware: the test passes because MCause==2 indicating illegal
  instruction trap.
- Vulnerable hardware (or if the redundant check is the only enforcement): the
  test fails because no illegal-in-U-mode trap was observed.

Notes & limitations:
- This test assumes the toolchain and runtime allow direct CSR writes to
  MSTATUS/MEPC and execution of mret from M-mode. The exact behavior depends on
  the platform's trap handling and test framework trap vectors. The test is
  designed to be robust: it checks MCause after returning to M-mode to
  determine whether an illegal instruction exception occurred.
- The PoC focuses on detection (POC), not on performing privilege escalation.

