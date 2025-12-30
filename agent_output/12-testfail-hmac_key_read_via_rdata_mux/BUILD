load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "hmac_vuln_read_test",
    srcs = ["hmac_vuln_read_test.c"],
    exec_env = {
        "//hw/top_earlgrey:sim_verilator": None,
        "//hw/top_earlgrey:sim_dv": None,
    },
    fpga = fpga_params(
        changes_otp = False,
    ),
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//hw/top:hmac_c_regs",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/base:abs_mmio",
        "//sw/device/lib/base:bitfield",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)
