load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_test(
    name = "lc_ctrl_hash_truncation_v3_test",
    srcs = ["lc_ctrl_hash_truncation_v3_test.c"],
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",  # Provides TOP_EARLGREY_*_BASE_ADDR constants
        "//hw/top:lc_ctrl_c_regs",  # Provides lc_ctrl_regs.h (CRITICAL)
        "//sw/device/lib/base:mmio",      # MMIO region operations
        "//sw/device/lib/base:bitfield",  # Bitfield manipulation
        "//sw/device/lib/runtime:log",    # LOG_INFO logging
        "//sw/device/lib/testing/test_framework:ottf_main",  # Test framework
    ],
)
