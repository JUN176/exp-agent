load("//rules/opentitan:defs.bzl", "opentitan_test", "fpga_params")

opentitan_test(
    name = "aes_force_masks_exploit_test",
    srcs = ["aes_force_masks_exploit_test.c"],
    exec_env = {
        "//hw/top_earlgrey:sim_verilator": None,
        "//hw/top_earlgrey:fpga_cw310_sival": None,
    },
    verilator = {
        # Keep longer timeout for crypto tests that rely on DV models.
        "timeout": "long",
    },
    deps = [
        "//hw/ip/aes:model",
        "//hw/top:dt",
        "//sw/device/lib/base:memory",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:aes",
        "//sw/device/lib/dif:csrng",
        "//sw/device/lib/dif:edn",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing:aes_testutils",
        "//sw/device/lib/testing:csrng_testutils",
        "//sw/device/lib/testing/test_framework:check",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)
