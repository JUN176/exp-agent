## agent2.py 使用说明

### 1. 角色与整体功能

`agent2.py` 是“第二阶段修复 Agent”的控制脚本，主要负责：

- **消费 `agent_output/` 中的漏洞利用包**：遍历每个漏洞目录，找到其中的 `.c` 和 `BUILD` 文件；
- **将 C 测试用例接入 OpenTitan Bazel 测试框架**：把生成的 C 文件拷贝到 `sw/device/tests`，并在其 `BUILD` 中添加/修正 `opentitan_test` 规则；
- **调用 Bazel 运行测试并自动修复**：使用一个专门的 `fixer_agent`，循环执行“运行测试 → 分析输出 → 修改 C 或 BUILD → 再跑一次”直到测试通过或无法再修复；
- **保证 Bazel 串行运行与心跳输出**：通过全局锁与 heartbeat 线程，避免并发 Bazel 冲突，并在长时间编译/仿真时周期性输出“仍在运行”的提示。

可以理解为：**第一阶段生成 PoC（`agent.py`） → 第二阶段让 PoC 在真实 OpenTitan Bazel 环境中跑通（`agent2.py`）**。

---

### 2. 路径与全局配置

```python
AGENT_OUTPUT_DIR = "/home/exp-agent/agent_output"
OPENTITAN_TESTS_DIR = "/home/exp-agent/opentitan/sw/device/tests"
OPENTITAN_ROOT = "/home/exp-agent/opentitan"
DEST_BUILD_FILE = os.path.join(OPENTITAN_TESTS_DIR, "BUILD")
```

- **`AGENT_OUTPUT_DIR`**：第一阶段 Agent 生成的漏洞利用包根目录；
- **`OPENTITAN_TESTS_DIR`**：OpenTitan C 测试所在目录；生成的 `.c` 会被复制到这里；
- **`OPENTITAN_ROOT`**：OpenTitan 仓库根目录，运行 Bazel 时的工作目录；
- **`DEST_BUILD_FILE`**：OpenTitan 测试目录下的主 `BUILD` 文件路径。

> 注意：当前路径都写死为 `/home/exp-agent/...`，如在本地环境不同，请根据实际安装位置修改这些常量。

#### 2.1 Bazel 全局锁

```python
BAZEL_LOCK = threading.Lock()
```

- 用于保证 **同一时刻只会有一个 Bazel 进程** 在跑测试；
- 若锁已被占用，新的 Bazel 测试请求会返回“Bazel is already running”的错误信息。

---

### 3. 工具函数

#### 3.1 `read_file(file_path: str) -> str`

- 功能：读取指定文件内容。
- 行为：
  - 文件不存在时返回 `"Error: File not found: ..."`；
  - 其他异常返回 `"Error reading file: ..."`；
  - 正常时返回文件全文字符串。
- 用途：供 `fixer_agent` 查看 C 源码或 BUILD 内容。

#### 3.2 `write_file(file_path: str, content: str) -> str`

- 功能：覆盖写入文件，用新内容替换旧内容。
- 行为：返回成功或错误消息字符串。
- 用途：由 `fixer_agent` 用来修复 C 文件或（在需要时）写回 BUILD 内容。

#### 3.3 `run_bazel_test(test_name: str) -> str`

- 功能：在 OpenTitan 根目录下执行单个 Bazel 测试，并返回“状态 + 截断后的完整输出”。
- 关键步骤：
  1. **规范化测试名**：
     - 若以 `"//"` 开头且包含 `":"`，只保留 `:` 后部分；
     - 若以 `_sim_verilator` 结尾，则先去掉这段，最后统一加上；
     - 构造目标：`//sw/device/tests:{test_name}_sim_verilator`。
  2. **构造命令**：
     ```bash
     bazel test --test_output=streamed -j 16 --test_timeout=9999 //sw/device/tests:XXX_sim_verilator
     ```
  3. **并发控制**：
     - 尝试非阻塞获取 `BAZEL_LOCK`；
     - 如果失败，直接返回“Bazel is already running”；
  4. **心跳线程**：
     - 每 20 秒检查一次最后一行 Bazel 输出，并打印“Still Running”提示，避免长时间无输出看起来像卡住；
  5. **流式读取输出**：
     - 使用 `subprocess.Popen` + 行流式读取；
     - 每行即刻打印到终端，并同时缓存在 `output_buffer` 中；
  6. **返回结果**：
     - 根据退出码设置状态：`PASS` 或 `FAIL (Exit Code N)`；
     - 拼接完整输出文本；
     - 若输出超过 50k 字符，则中间截断，只保留前后各 25k 以避免上下文过长。
- 错误处理：
  - 若过程中出现异常，会尝试终止/kill Bazel 子进程，并返回 `EXECUTION ERROR: ...`。

#### 3.4 `update_build_rule(test_name: str, new_rule_content: str) -> str`

- 功能：在 `sw/device/tests/BUILD` 中，用新的 `opentitan_test` 规则替换指定 `name` 的旧规则。
- 实现细节：
  1. 读入 `DEST_BUILD_FILE` 全文；
  2. 通过正则 `opentitan_test\s*\(\s*name\s*=\s*"{test_name}"` 找到目标规则起始位置；
  3. 从该位置开始，按括号计数法找到对应的闭合 `)` 位置；
  4. 用 `new_rule_content` 替换原有规则块；
  5. 写回 `BUILD` 文件。
- 返回值：成功或错误提示。
- 用途：
  - 当 Bazel 报出依赖缺失、规则错误等问题时，`fixer_agent` 可以构造新的 `opentitan_test` 规则，并通过此工具替换原有规则。

---

### 4. 修复 Agent：`fixer_agent`

```python
system_prompt = """
You are an expert Embedded Systems Engineer and Bazel Build Master specializing in OpenTitan.
...
"""

fixer_agent = Agent(
    model,
    system_prompt=system_prompt,
    tools=[read_file, write_file, run_bazel_test, update_build_rule],
)
```

#### 4.1 模型与职责

- 使用与 `agent.py` 类似的 `OpenAIModel` + `OpenAIProvider`，但模型名为 `"gpt-5-mini"`；
- system prompt 定位为：
  - **“OpenTitan 嵌入式测试 + Bazel Build 专家”**；
  - 目标是：**让给定测试在 Bazel 下通过**。

#### 4.2 工作流程（由提示词约束）

1. 通过 `run_bazel_test(test_name)` 跑一次测试；
2. 分析结果：
   - 若通过：返回成功信息并停止；
   - 若失败：
     - 用 `read_file` 读取 C 文件；
     - （必要时）读取 BUILD 内容或从上下文推断；
     - 判断失败类型（编译错误、链接错误、运行时断言等）；
     - 通过 `write_file` 或 `update_build_rule` 修正代码/BUILD；
     - 再次回到步骤 1 重试；
3. 修复策略注意事项：
   - 保留漏洞测试的核心逻辑，不要简单删除断言；
   - BUILD 依赖缺失时，应补充到 `opentitan_test.deps` 中；
   - 测试超时时，检查是否有死循环或超低 timeout 设置；
   - 修改前必须先读取原文件，避免误删上下文。

---

### 5. 单个漏洞目录处理：`process_vulnerability(vuln_dir)`

```python
async def process_vulnerability(vuln_dir):
    ...
```

#### 5.1 输入与目录结构

- **`vuln_dir`**：`AGENT_OUTPUT_DIR` 下的单个目录名，一般形如：`"15-fail-aes_reset_retention"`；
- 对应目录内部应至少包含：
  - 一个 `.c` 文件（漏洞 PoC 测试）；
  - 一个 `BUILD` 文件（原始构建规则）；
  - 可能还有 `README.md` 及其他文件。

#### 5.2 步骤概览

1. **定位文件**：
   - 遍历目录，找到第一个 `.c` 文件和名为 `BUILD` 的文件；
   - 若缺任一，则跳过此目录。
2. **复制 C 文件到 OpenTitan 测试目录**：
   - 源路径：`vuln_path/<c_file>`；
   - 目标路径：`OPENTITAN_TESTS_DIR/<c_file>`；
   - 失败则打印错误并返回。
3. **解析原始 BUILD 规则**：
   - 从漏洞目录内的 `BUILD` 文件中抽取 `opentitan_test(...)` 规则；
   - 使用括号计数法，从匹配到 `opentitan_test(` 的位置开始找到对应的结束 `)`；
   - 若找不到起点或完整括号，说明规则异常，跳过该目录。
4. **从规则中提取 `srcs` 与 `name`**：
   - 用正则查找 `srcs = ["xxx.c"]`，得到 C 文件名；
   - 推导期望的测试名：`expected_name = c_filename.replace(".c", "")`；
   - 用正则查找 `name = "..."` 获取当前名称；
   - 若当前名称与期望不一致，则替换为期望名称；
   - `test_name` 设为期望名称，用于后续 Bazel 目标和 Agent 提示。
5. **将规则追加到 OpenTitan `BUILD` 中**：
   - 读入 `DEST_BUILD_FILE`；
   - 若不存在同名 `name = "test_name"` 的规则，则在文件末尾追加：
     ```python
     \n\n# Added by agent2.py for {vuln_dir}\n{rule_content}
     ```
   - 否则打印提示“已存在规则”。
6. **调用 `fixer_agent` 修复/验证**：
   - 构造 Prompt：告知测试名、C 文件路径、BUILD 路径，并要求：
     > 先运行测试，若失败则修复代码/BUILD，直到测试通过。
   - 执行 `result = await fixer_agent.run(prompt)`；
   - 打印 Agent 的结果报告。

---

### 6. 主函数：`main()` 与命令行用法

```python
async def main():
    ...

if __name__ == "__main__":
    asyncio.run(main())
```

#### 6.1 前置检查

- 确认 `AGENT_OUTPUT_DIR` 目录存在；
- 确认 `OPENTITAN_TESTS_DIR` 目录存在；
- 若不存在则打印错误并退出。

#### 6.2 备份 OpenTitan BUILD 文件

- 若 `DEST_BUILD_FILE + ".bak"` 不存在，则复制原始 `BUILD` 为 `.bak` 备份。
- 确保在批量追加新测试规则前有可恢复版本。

#### 6.3 收集需要处理的漏洞目录

- 列出 `AGENT_OUTPUT_DIR` 下所有子目录，并按字典序排序；
- 若命令行带参数：
  - `python agent2.py <vuln_id>`：只处理目录名以 `<vuln_id>-` 开头或等于 `<vuln_id>` 的目录；
- 若无参数：
  - 处理所有漏洞目录。

#### 6.4 逐个调用 `process_vulnerability`

- 当前实现为 **串行** 处理：
  ```python
  for vuln_dir in vuln_dirs:
      await process_vulnerability(vuln_dir)
  ```
- 考虑到 Bazel 本身较重且通过全局锁限制并发，相比并发，多数情况下串行处理更稳定。

---

### 7. 实际使用示例

#### 7.1 处理所有漏洞

```bash
python agent2.py
```

- 前提：`AGENT_OUTPUT_DIR` 中已经有 `agent.py` 生成的多个漏洞子目录；
- 脚本会：
  - 备份 `sw/device/tests/BUILD`；
  - 逐个目录复制 `.c` 文件并追加 `opentitan_test` 规则；
  - 调用 `fixer_agent` 运行 Bazel 并尝试修复直到测试通过。

#### 7.2 只处理指定漏洞 ID

```bash
python agent2.py 15
```

- 只会匹配形如：
  - `"15-..."` 或目录名恰好为 `"15"`；
- 适合针对某个特定漏洞单独调试和迭代。

---

### 8. 注意事项与扩展建议

- **路径适配**：
  - 如果你的项目目录不在 `/home/exp-agent/`，请务必修改文件顶部的三个路径常量。
- **Bazel 环境**：
  - 确保已在 `OPENTITAN_ROOT` 下正确初始化 Bazel，能手动运行 `bazel test`；
- **安全回滚**：
  - 所有对 `sw/device/tests/BUILD` 的修改都基于 `.bak` 备份，出问题时可以手动恢复；
- **扩展工具**：
  - 如果需要更复杂的 BUILD 结构或多测试目标，可以扩展 `update_build_rule` 或增加新工具函数，让 `fixer_agent` 有更多操作能力；
- **调试 `fixer_agent` 行为**：
  - 如需约束其修改范围或风格，可直接调整 `system_prompt` 中的规则与示例。
