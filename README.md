# Exp-Agent

Exp-Agent 是一个专为 OpenTitan 生态系统设计的自动化硬件漏洞分析与验证工具。它利用大语言模型（LLM）深度分析硬件 RTL 代码中的漏洞，并自动生成高质量的 C 语言验证测试（PoC），以证明漏洞的存在。

## 主要功能

- **自动化漏洞分析**: 能够读取并理解 Verilog/SystemVerilog RTL 代码，定位漏洞根源。
- **智能上下文感知**: 自动检索 OpenTitan 项目中的相关软件库（Base Libs, DIFs）、测试用例和构建文件。
- **PoC 生成**: 生成可编译、可运行的 C 语言测试代码（必要时包含汇编），用于复现漏洞。
- **构建系统集成**: 自动生成 Bazel `BUILD` 文件，确保生成的测试可以直接在 OpenTitan 仿真环境中运行。
- **并发处理**: 支持并发分析多个漏洞，提高效率。

## 项目结构

```text
exp-agent/
├── agent.py            # 核心代理逻辑，负责与 LLM 交互和处理漏洞分析任务
├── agent2.py           # 扩展代理逻辑，包含自动运行 Bazel 测试的功能
├── tools.py            # 工具库，提供文件查找、读取、RTL 分析和 PoC 包生成等功能
├── system_prompt.txt   # LLM 的系统提示词，定义了代理的角色、工作流和编码规范
├── pyproject.toml      # 项目依赖和配置管理
├── opentitan/          # OpenTitan 源码仓库（子模块或副本）
├── agent_output/       # 输出目录，存放生成的 PoC 代码和分析报告
└── vuln_report/        # 漏洞报告数据
```

## 环境要求

- Python >= 3.12
- OpenTitan 开发环境（Bazel, Verilator 等）
- 有效的 OpenAI 兼容 API Key

## 安装与配置

1.  **安装依赖**:
    本项目使用 `uv` 或 `pip` 进行依赖管理。

    ```bash
    pip install .
    ```

    或者直接安装 `pyproject.toml` 中列出的依赖：

    - `dotenv`
    - `openai`
    - `pydantic-ai`

2.  **配置环境变量**:
    在项目根目录下创建 `.env` 文件，并填入你的 API Key 和 Base URL：
    ```env
    OPEN_API_KEY=your_api_key_here
    # 根据 agent.py 中的配置，可能还需要配置 base_url
    ```

## 使用说明

### 运行代理

主要通过 `agent.py` 或 `agent2.py` 启动分析任务。

- **agent.py**: 专注于漏洞分析和代码生成。它会读取漏洞列表，为每个漏洞创建一个独立的 Agent 实例进行分析。
- **agent2.py**: 包含更完整的流程，支持生成后直接调用 Bazel 进行仿真测试 (`run_bazel_test`)。

### 输出结果

生成的 Exploit PoC 会保存在 `agent_output/` 目录下。每个漏洞会有独立的文件夹（例如 `1-success-aes_keyshare1_read_alias/`），通常包含：

- `vulnerability.c`: 漏洞验证 C 代码
- `BUILD`: Bazel 构建文件
- `README.md`: 漏洞复现步骤说明
- `*.S`: (可选) 汇编辅助代码

## 输入与输出

### 输入 (Inputs)

- **漏洞列表**: `vuln_report/vuln.csv`
  - Agent 会读取此 CSV 文件，获取待分析的漏洞信息（如漏洞 ID、模块名、文件路径、行号等）。
- **系统提示词**: `system_prompt.txt`
  - 定义了 Agent 的角色、任务目标、思维链（CoT）步骤以及输出格式规范。
- **环境变量**: `.env`
  - 包含 LLM API Key (`OPEN_API_KEY`) 和 Base URL 等配置。
- **OpenTitan 源码**: `opentitan/` 目录
  Agent 会根据漏洞信息动态读取以下具体文件作为上下文：
  - **Base Library 文档 (核心)**: `sw/device/lib/base/BASE_LIB_REFERENCE.md` (API 参考手册)。
  - **RTL 源代码**: 漏洞报告中指定的 `.sv` 文件（如 `hw/ip/aes/rtl/aes_cipher_core.sv`）。
  - **寄存器定义文件 (关键)**: `*_regs.h` 文件（如 `kmac_regs.h`, `aes_regs.h`）。
    - 这些文件包含所有寄存器偏移量 (`_REG_OFFSET`) 和位掩码 (`_BIT`, `_MASK`)。
    - 通常位于 `hw/top_earlgrey/sw/autogen/` 或构建输出目录中。
    - Agent 必须读取这些文件以获取准确的硬件地址宏，避免幻觉。
  - **顶层配置文件**: `hw/top_earlgrey/sw/autogen/top_earlgrey.h`。
    - 提供模块的物理基地址宏 (如 `TOP_EARLGREY_AES_BASE_ADDR`)。
  - **DIF 头文件**: `sw/device/lib/dif/dif_<module>.h` (仅用于查看寄存器定义)。
  - **Testutils 库**: `sw/device/lib/testing/<module>_testutils.h` (查看现有的测试辅助函数)。
  - **现有测试用例**: `sw/device/tests/<module>_*.c` (参考测试框架用法)。
  - **构建文件**: `sw/device/tests/BUILD` (分析依赖关系)。
  - **Base Headers**: `sw/device/lib/base/*.h` (如 `mmio.h`, `bitfield.h` 等底层接口)。

### 输出 (Outputs)

- **Exploit 包**: `agent_output/` 目录下生成的各个漏洞文件夹。
  - 每个文件夹命名格式通常为 `{id}-{status}-{name}`。
  - 包含文件：
    - `vulnerability.c`: 核心漏洞验证代码。
    - `BUILD`: 对应的 Bazel 构建规则。
    - `README.md`: 该漏洞的复现指南。
    - `*.S`: (如有) 辅助汇编代码。
- **执行日志**: 控制台输出的实时进度、Agent 思考过程摘要以及最终的统计报告（成功/失败数量、耗时）。

## Agent 构建流程

当运行 `agent.py` 时，构建和初始化过程如下：

1.  **加载配置**: 读取 `.env` 文件加载环境变量。
2.  **加载提示词**: 读取 `system_prompt.txt` 内容，作为 Agent 的 System Message。
3.  **加载工具**: 导入 `tools.py` 中定义的工具函数 (`find_related_files`, `read_file_content`, `read_rtl_lines`, `create_exploit_package`) 并注册给 Agent。
4.  **读取任务**: 调用 `tools.read_vulnerability_csv("vuln_report/vuln.csv")` 解析漏洞列表。
5.  **并发初始化**: 为列表中的每个漏洞创建一个**全新**的 Agent 实例（无历史记忆），并发执行分析任务。

## 开发指南

- **Prompt Engineering**: 修改 `system_prompt.txt` 可以调整代理的行为模式和编码风格。
  - **Base Lib vs DIF**: 目前设定为**强制**优先使用 Base Library (`abs_mmio`, `mmio` 等)。
  - **原因**: DIF (Device Interface Functions) 封装层级过高，通常包含状态检查和断言，限制了操作的灵活性。硬件漏洞往往需要非预期的寄存器操作序列（如乱序访问、非法值写入），底层 Base Lib 提供了更灵活的排列组合能力，能够构造出 DIF 无法实现的复杂 PoC。
- **工具扩展**: 在 `tools.py` 中添加新的 Python 函数可以为代理赋予新的能力（如读取波形、查询文档等）。
