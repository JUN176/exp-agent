load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "aes_vuln_readback_test",
    srcs = ["aes_vuln_readback_test.c"],
    exec_env = {
        "//hw/top_earlgrey:sim_verilator": None,
        "//hw/top_earlgrey:sim_dv": None,
    },
    fpga = fpga_params(
        changes_otp = False,
    ),
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",  # Provides TOP_EARLGREY_AES_BASE_ADDR
        "//hw/top:aes_c_regs",                       # Provides aes_regs.h
        "//sw/device/lib/base:mmio",                 # MMIO operations
        "//sw/device/lib/base:bitfield",             # Bitfield ops (if needed)
        "//sw/device/lib/runtime:log",               # LOG_INFO
        "//sw/device/lib/testing/test_framework:ottf_main",  # Test framework
    ],
)
