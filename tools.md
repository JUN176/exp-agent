## tools.py 工具模块说明

### 1. 模块整体概览

`tools.py` 是 Agent 的通用工具库，主要负责：

- **OpenTitan 代码定位**：根据模块名快速定位 base 库、DIF、testutils、测试用例和 BUILD 文件等路径。
- **文件内容读取**：按整文件、行区间或复杂行号规格读取 C/RTL 等源码内容。
- **漏洞批量导入**：从 CSV 报告中解析结构化的漏洞列表，作为后续自动化处理的输入。
- **漏洞利用包打包输出**：按统一目录结构生成漏洞 PoC/Exploit 包，并记录工具调用日志，方便复现与审计。
- **工具使用日志记录**：通过 `ContextVar` 记录每次工具调用，最终写入 `tool_usage.log` 文件。

整体上，`tools.py` 把对磁盘文件的各种操作进行了封装，Agent 通过这些函数来“看见” OpenTitan 代码库并产出标准化的输出目录。

---

### 2. 全局状态与内部辅助工具

#### 2.1 `tool_usage_log: ContextVar[Optional[List[str]]]`

- **作用**：在当前执行上下文中保存工具调用日志，用于记录 Agent 在一次任务中的行为轨迹。
- **类型**：`ContextVar[Optional[List[str]]]`，默认值为 `None`。
- **典型使用方式**：
  - 在上层逻辑中，可以在一次任务开始前设置：
    ```python
    from contextvars import ContextVar
    tool_usage_log.set([])  # 开启本次任务的工具调用记录
    ```
  - 各个工具函数调用 `_log_action(...)` 时会往该列表追加一条带时间戳的日志。
  - 在 `create_exploit_package` 中，如果存在日志列表，会把它写到 `tool_usage.log` 中。

#### 2.2 `_log_action(message: str)`

- **定位**：内部辅助函数，不对外暴露。
- **功能**：把一条带时间戳的日志写入当前上下文中的 `tool_usage_log` 列表。
- **行为细节**：
  - 如果 `tool_usage_log` 为 `None`，则什么也不做（不会报错）。
  - 日志格式大致为：`[YYYY-MM-DD HH:MM:SS] message`。
- **使用场景**：所有对外工具（如 `find_related_files`、`read_file_content`、`read_rtl_lines`、`create_exploit_package`）都会调用 `_log_action` 记录自己被调用的参数，有利于后续审计。

#### 2.3 `_search_and_match(search_path: str, regex_pattern: str) -> List[str]`

- **定位**：内部文件搜索工具，为 `find_related_files` 提供底层支持。
- **功能**：
  - 在给定目录 `search_path` 下递归遍历所有文件，
  - 使用 `regex_pattern`（正则表达式）匹配 **文件名**，
  - 返回所有匹配文件的归一化路径列表（路径分隔符统一为 `/`）。
- **返回值**：`List[str]`，每个元素是匹配文件的相对路径，例如：`"opentitan/sw/device/lib/base/mem.h"`。
- **错误处理**：
  - 若 `search_path` 不是目录，则直接返回空列表。
  - 若正则表达式非法，会打印错误提示并返回空列表，而不是抛异常。
- **适用场景**：
  - 把“在哪个目录、按什么规则找文件”的逻辑抽象出来，简化上层工具接口。

---

### 3. OpenTitan 相关文件查找：`find_related_files`

```python
find_related_files(
    module_name: str,
    file_type: Literal['dif', 'testutils', 'tests', 'build', 'base', 'all']
) -> List[str]
```

#### 3.1 功能概述

- **核心作用**：根据 OpenTitan 模块名（如 `"otp_ctrl"`、`"aes"` 等），在约定好的目录结构里自动搜索：
  - base 库文件
  - DIF（Driver Interface）文件
  - testutils 测试工具库文件
  - tests 测试用例代码
  - tests 下的 BUILD 文件
- **意义**：把 OpenTitan 复杂、冗长的路径和命名约定封装在一个函数中，让 Agent 只需关心 `module_name` 与 `file_type`。

#### 3.2 参数说明

- **`module_name`**：
  - 模块名字符串，例如：`"otp_ctrl"`、`"aes"`、`"hmac"`。
  - 用于拼接正则表达式，过滤与该模块相关的文件名。

- **`file_type`**：
  - 可选值：`'base' | 'dif' | 'testutils' | 'tests' | 'build' | 'all'`。
  - 不同类型对应的搜索范围与命名规则如下：

| file_type   | 搜索目录                                      | 文件名正则规则                                    | 说明                                          |
|------------|----------------------------------------------|--------------------------------------------------|-----------------------------------------------|
| `base`     | `opentitan/sw/device/lib/base`              | `^.*\.(h|c)$`                                   | 所有 base 库的 `.c`/`.h` 文件，**不限定模块名** |
| `dif`      | `opentitan/sw/device/lib/dif`               | `^dif_{module_name}.*\.(h|c)$`                 | 针对指定模块名的 DIF 文件                      |
| `testutils`| `opentitan/sw/device/lib/testing`           | `^{module_name}_testutils.*\.(h|c)$`           | 该模块对应的测试工具库                        |
| `tests`    | `opentitan/sw/device/tests`                 | `^{module_name}.*\.(c|h)$`                     | 以模块名开头的测试源码/头文件                  |
| `build`    | `opentitan/sw/device/tests`                 | `^BUILD$`                                      | 测试目录下的 BUILD 文件                        |
| `all`      | 递归调用上述 5 类                           | 组合去重后的所有匹配结果                        | 一次性获取所有相关文件路径                    |

#### 3.3 返回值

- **类型**：`List[str]`
- **内容**：匹配到的文件相对路径列表（用 `/` 作为分隔符），若无匹配或 `file_type` 非法，则返回空列表。

#### 3.4 错误处理

- 若 `file_type` 不在预期集合中：
  - 会打印错误提示：
    > Error: Invalid file_type 'xxx'. Must be one of: 'base', 'dif', 'testutils', 'tests', 'build', 'all'.
  - 返回空列表。

#### 3.5 典型用法示例

```python
# 获取 AES 模块相关的 DIF 文件
paths = find_related_files("aes", "dif")

# 一次性获取 HMAC 模块相关的所有文件
hmac_files = find_related_files("hmac", "all")

# 遍历打印
for p in hmac_files:
    print(p)
```

---

### 4. 源码读取工具：`read_file_content`

```python
read_file_content(
    file_path: str,
    start_line: Optional[int] = None,
    end_line: Optional[int] = None
) -> str
```

#### 4.1 功能概述

- **核心作用**：读取指定路径文件的内容，支持：
  - 读取整个文件；
  - 按行区间读取子片段（例如只读第 10~50 行）。
- **典型应用**：
  - Agent 想了解某个库/测试文件的全部 API 定义；
  - 只关注某个漏洞相关代码片段时，只读取给定行区间，减少无关噪音。

#### 4.2 参数说明

- **`file_path`**：
  - 待读取文件的相对路径或绝对路径。
  - 通常会直接使用 `find_related_files` 的结果，例如：
    `"opentitan/sw/device/lib/dif/dif_otp_ctrl.h"`。

- **`start_line`**（可选）：
  - 起始行号，**从 1 开始计数**，包含这一行。
  - 若为 `None`，表示从文件开头开始读取。

- **`end_line`**（可选）：
  - 结束行号，从 1 开始计数，包含这一行。
  - 若为 `None`，表示一直读到文件末尾。

#### 4.3 返回值

- 正常情况下：返回一个字符串，内容为所请求的文件片段。
- 出错时：返回带有 `"Error: ..."` 前缀的错误描述字符串，例如：
  - 文件不存在或路径为目录；
  - 行号非法（非正数，起始行大于结束行，起始行超出文件长度等）。

#### 4.4 校验与错误处理

- 若 `file_path` 不是一个存在的普通文件：
  - 返回：`"Error: File not found or path is a directory: 'xxx'"`。
- 若 `start_line <= 0` 或 `end_line <= 0`：
  - 返回对应的错误字符串，而不是抛异常。
- 若 `start_line` > `end_line`：
  - 返回：`"Error: start_line (x) cannot be greater than end_line (y)."`。
- 若 `start_line` 超出文件总行数：
  - 返回：`"Error: start_line (x) is beyond the end of the file (total lines: N)."`。

#### 4.5 使用示例

```python
# 读取整个文件
content = read_file_content("opentitan/sw/device/lib/base/mem.h")

# 读取第 100~150 行
snippet = read_file_content("opentitan/sw/device/tests/aes_vuln_test.c", 100, 150)
print(snippet)
```

---

### 5. RTL 行读取工具：`read_rtl_lines`

```python
read_rtl_lines(file_path: str, line_spec: str) -> str
```

#### 5.1 功能概述

- **核心作用**：按“复杂行号规格”从 RTL 文件中抽取若干行，并在每行前加上行号前缀。
- **典型行号规格**：
  - 支持逗号分隔的多个单行号：`"729,770,811,852"`
  - 支持区间：`"1710-1724"`
  - 支持组合：`"729,770,811,852,1710-1724"`
- **输出格式**：
  - 每行都带 `Lxxxx:` 前缀（4 位零填充），例如：
    ```
    L0729: assign ...
    L0770: always_ff @(posedge clk) begin
    ```

#### 5.2 参数说明

- **`file_path`**：
  - RTL 文件路径（相对或绝对）。

- **`line_spec`**：
  - 行号表达式字符串，支持：
    - 单个行号（如 `"100"`）；
    - 行号列表（如 `"100,105,110"`）；
    - 行号区间（如 `"200-220"`）；
    - 组合（如 `"100,150-160,200"`）。

#### 5.3 行号解析与排序

- 函数内部会：
  - 把 `line_spec` 按逗号拆分；
  - 对每个片段：
    - 若包含 `-`，解析为区间 `[start, end]`，并展开为所有行号；
    - 否则解析为单行号；
  - 若解析失败（非数字/格式错误），会立即返回错误字符串；
  - 去重并按升序排序，保证输出行号顺序稳定。

#### 5.4 返回值与错误处理

- 正常情况下：
  - 返回拼接好的多行字符串，每行前带 `Lxxxx:` 前缀。
- 当无法读取文件时：
  - 返回：`"Error: Could not read file 'xxx'. Reason: ..."`。
- 当某个请求的行号超出文件行数时：
  - 对该行输出占位提示，例如：
    ```
    L9999: [Line not found in file (file has 1200 lines)]
    ```

#### 5.5 使用示例

```python
spec = "729,770,811,852,1710-1724"
rtl_snippet = read_rtl_lines("opentitan/hw/ip/aes/rtl/aes_core.sv", spec)
print(rtl_snippet)
```

---

### 6. 漏洞 CSV 读取工具：`read_vulnerability_csv`

```python
read_vulnerability_csv(csv_path: str) -> List[dict]
```

#### 6.1 功能概述

- **核心作用**：从漏洞报告 CSV 文件中读取所有记录，并转换为结构化的字典列表。
- **典型用途**：
  - 批量处理漏洞：Agent 可以一次性导入所有漏洞信息，再针对每条记录生成 PoC/Exploit。

#### 6.2 CSV 预期结构

- 期望的标题行（schema）：
  - `vuln-id,filepath,moudle,line,Finding,Security impact`
- 注意：
  - 字段名中存在拼写：`moudle`（不是 `module`），代码里专门按 `moudle` 读取。

#### 6.3 返回数据结构

- 返回值类型：`List[dict]`
- 每个元素（漏洞记录字典）包含字段：
  - **`vuln_id`**：来自 CSV 的 `vuln-id`，去掉首尾空白。
  - **`filepath`**：来自 `filepath` 列，对应 RTL 文件路径。
  - **`module`**：来自 `moudle` 列，对应模块名。
  - **`line`**：来自 `line` 列，通常是行号或行号规格字符串。
  - **`finding`**：来自 `Finding` 列，对漏洞描述或具体问题的文本。
  - **`security_impact`**：来自 `Security impact` 列，对安全影响的说明。

#### 6.4 错误处理与健壮性

- 使用 `utf-8-sig` 编码读取，自动处理 BOM；`errors='replace'` 确保遇到异常字符时不会崩溃。
- 对于缺失字段，通过内部 `safe_strip` 函数统一做 `None` 安全处理（返回空字符串）。
- 会跳过 `vuln-id` 为空或只含空白的行，避免把空行当成有效漏洞记录。
- 若文件不存在：
  - 打印 `Error: CSV file not found: ...`，返回空列表。
- 其他异常：
  - 打印 `Error reading CSV file: ...`，返回空列表。

#### 6.5 使用示例

```python
vulns = read_vulnerability_csv("vuln_report/vuln.csv")
for v in vulns:
    print(v["vuln_id"], v["filepath"], v["line"], v["finding"])
```

---

### 7. 漏洞利用包生成工具：`create_exploit_package`

```python
create_exploit_package(
    vulnerability_name: str,
    c_filename: Optional[str] = None,
    c_content: Optional[str] = None,
    asm_filename: Optional[str] = None,
    asm_content: Optional[str] = None,
    build_content: Optional[str] = None,
    readme_content: Optional[str] = None,
    output_base_dir: str = "agent_output",
    vuln_id: Optional[str] = None
) -> str
```

#### 7.1 功能概述

- **核心作用**：
  - 为单个漏洞生成一个完整的“漏洞利用包”目录，
  - 写入 C/ASM 源码、BUILD 配置、README 说明和工具使用日志，
  - 形成可直接构建和复现的 PoC/Exploit 包。
- **目录结构**：
  - 默认在 `agent_output/` 下创建子目录：
    - 若提供 `vuln_id`：
      - 目录名：`{vuln_id}-{vulnerability_name}`，例如：`"15-fail-aes_reset_retention"`。
    - 若未提供 `vuln_id`：
      - 目录名：`{vulnerability_name}`。
  - 目录内部典型结构：
    ```
    agent_output/
      └── {vuln_id}-{vulnerability_name}/
          ├── BUILD
          ├── <c_filename>        (可选)
          ├── <asm_filename>      (可选)
          ├── README.md           (可选)
          └── tool_usage.log      (若有日志)
    ```

#### 7.2 参数说明

- **`vulnerability_name`**（必选）：
  - 漏洞名称或短标签，用于目录命名的一部分，例如 `"aes_reset_retention"`。

- **`c_filename` / `c_content`**（可选）：
  - 若两者均提供，则会在漏洞目录中写入对应 C 文件：
    - 路径：`{dir}/<c_filename>`。
  - 若其中任一缺失，则不会写入 C 文件。

- **`asm_filename` / `asm_content`**（可选）：
  - 类似 C 文件逻辑，通常用于裸汇编 Exploit。

- **`build_content`**（可选）：
  - 若提供，则写入 `BUILD` 文件，作为构建规则。

- **`readme_content`**（可选）：
  - 若提供，则写入 `README.md`，用于记录漏洞背景、触发步骤、预期行为、复现命令等。

- **`output_base_dir`**（可选）：
  - 输出基础目录，默认 `"agent_output"`，可根据需求调整（例如改为其他输出根目录）。

- **`vuln_id`**（可选）：
  - 若提供，则作为目录前缀，与 `vulnerability_name` 拼接，方便在大量漏洞中按 ID 快速定位。

#### 7.3 工具使用日志写入

- 在创建目录后，函数会尝试从 `tool_usage_log` 中读取日志列表：
  - 若存在且非空，则写入 `tool_usage.log`：
    - 每行一条日志，内容即 `_log_action` 记录的字符串。
  - 这使得每个 Exploit 包自带“生成过程”的审计信息。

#### 7.4 返回值与错误处理

- 成功时：
  - 返回带有 `"✓ Exploit package created successfully at: ..."` 开头的多行字符串，列出所有创建的文件名。
- 若过程中发生异常：
  - 捕获异常并返回：`"Error creating exploit package: ..."`。

#### 7.5 使用示例

```python
from tools import create_exploit_package

result = create_exploit_package(
    vulnerability_name="aes_reset_retention",
    c_filename="aes_vuln_test.c",
    c_content="""// C PoC code here...""",
    build_content="""# BUILD rules here...""",
    readme_content="# AES reset retention vuln\n\nSteps to reproduce...",
    output_base_dir="agent_output",
    vuln_id="15-fail-aes_reset_retention"
)

print(result)
```

---

### 8. 典型工作流示例：从漏洞 CSV 到 Exploit 包

下面是一个串联 `tools.py` 中多个工具的典型工作流示例：

1. **读取漏洞列表**：
   ```python
   vulns = read_vulnerability_csv("vuln_report/vuln.csv")
   ```

2. **对每个漏洞，读取 RTL 关键行**：
   ```python
   for v in vulns:
       rtl_snippet = read_rtl_lines(v["filepath"], v["line"])
       # 在这里可以对 rtl_snippet 做进一步分析
   ```

3. **根据模块名查找相关 C 测试文件或 DIF**：
   ```python
   related_tests = find_related_files(v["module"], "tests")
   ```

4. **生成 Exploit 包**：
   ```python
   result = create_exploit_package(
       vulnerability_name=v["module"],
       c_filename="{}_vuln_test.c".format(v["module"]),
       c_content="// 这里是自动生成或人工编写的 PoC 代码...",
       build_content="# 这里是对应的 BUILD 规则...",
       readme_content="漏洞描述: {}\n安全影响: {}".format(
           v["finding"], v["security_impact"]
       ),
       vuln_id=v["vuln_id"]
   )
   print(result)
   ```

通过以上工具组合，`tools.py` 为 Agent 提供了从“漏洞报告解析 → RTL/软件代码理解 → 漏洞 PoC 输出”的完整支撑能力。
