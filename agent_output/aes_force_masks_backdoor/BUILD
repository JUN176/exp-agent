load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "aes_force_masks_exploit_test",
    srcs = ["aes_force_masks_exploit_test.c"],
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
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:dif_aes",
        "//sw/device/lib/dif:dif_csrng",
        "//sw/device/lib/dif:dif_edn",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing:aes_testutils",
        "//sw/device/lib/testing:csrng_testutils",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)
