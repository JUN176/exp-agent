# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

load("//rules/opentitan:defs.bzl", "opentitan_test")

opentitan_test(
    name = "lc_ctrl_token_truncation_test",
    srcs = ["lc_ctrl_token_truncation_test.c"],
    exec_env = {"//hw/top_earlgrey:sim_dv": None},
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//hw/top:lc_ctrl_c_regs",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/base:abs_mmio",
        "//sw/device/lib/base:bitfield",
        "//sw/device/lib/base:memory",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)