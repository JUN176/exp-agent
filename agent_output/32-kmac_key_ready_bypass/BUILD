load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "kmac_v32_poc_test",
    srcs = ["kmac_key_ready_bypass_poc.c"],
    exec_env = {
        "//hw/top_earlgrey:sim_verilator": None,
        "//hw/top_earlgrey:sim_dv": None,
    },
    fpga = fpga_params(
        changes_otp = False,
    ),
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//hw/top:kmac_c_regs",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/base:bitfield",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)
