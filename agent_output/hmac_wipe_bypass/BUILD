load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "hmac_wipe_bypass_test",
    srcs = ["hmac_wipe_bypass_test.c"],
    exec_env = {
        "//hw/top_earlgrey:fpga_cw310_sival": None,
        "//hw/top_earlgrey:fpga_cw310_rom_with_fake_keys": None,
        "//hw/top_earlgrey:sim_dv": None,
        "//hw/top_earlgrey:sim_verilator": None,
    },
    fpga = fpga_params(
        changes_otp = False,
    ),
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/base:macros",
        "//sw/device/lib/base:memory",
        "//sw/device/lib/dif:hmac",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing:hmac_testutils",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)
