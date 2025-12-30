.section .text
.global enter_user_mode
.global user_func
.global trap_handler
.global user_result
.global target_data
.global after_user

// enter_user_mode(void *fn)
// a0: pointer to user function
.type enter_user_mode, @function
enter_user_mode:
    // Set mepc to the user function address passed in a0
    csrw mepc, a0
    // return from M-mode -> will go to the address in mepc with current MPP
    mret

// User-mode function that attempts to write to target_data and then ecall to
// notify M-mode that it completed.
.type user_func, @function
user_func:
    // Load address of target_data
    la t0, target_data
    li t1, 0xDEADBEEFu
    // Attempt write; if PMP blocks this it will trap before the ecall
    sw t1, 0(t0)
    // If we reach here, write succeeded. Use ecall to trap back to M-mode to
    // indicate success.
    ecall
1:  j 1b

// Trap handler in M-mode. Distinguish ECALL from U-mode (success) vs access
// faults (failure). Then set mepc to after_user label and return to M-mode.
.type trap_handler, @function
trap_handler:
    // t0 = mcause
    csrr t0, mcause
    // Compare to ECALL from U-mode (8)
    li t1, 8
    beq t0, t1, handle_ecall

    // Otherwise treat as access fault: set user_result = 0
    la t2, user_result
    li t3, 0
    sw t3, 0(t2)
    j finish

handle_ecall:
    // ECALL indicates the user function completed -> success
    la t2, user_result
    li t3, 1
    sw t3, 0(t2)

finish:
    // Set mepc to after_user so that mret will return to M-mode at that label
    la t4, after_user
    csrw mepc, t4

    // Set MSTATUS.MPP = M (binary 10) so that mret returns in M-mode
    csrr t5, mstatus
    // Clear MPP bits [12:11]
    li t6, - (3 << 11)
    and t5, t5, t6
    // Set MPP = 2 (M)
    li t6, (2 << 11)
    or t5, t5, t6
    csrw mstatus, t5

    // Return from trap; because we set mepc to after_user and MPP=M we will
    // continue in M-mode at the after_user label.
    mret

// A label for resuming execution in M-mode after the U-mode attempt
.type after_user, @function
after_user:
    ret
