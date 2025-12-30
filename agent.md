## agent.py 使用说明

### 1. 角色与整体功能

`agent.py` 是“漏洞利用包生成 Agent”的主入口，负责：

- **批量读取漏洞报告**：从 `vuln_report/vuln.csv` 中加载所有漏洞记录；
- **为每个漏洞启动独立会话的 AI Agent**：使用 `pydantic_ai.Agent` + 自定义 `system_prompt`；
- **调用工具链 `tools.py`**：读取 RTL、查找 OpenTitan 相关 C/测试文件，并生成漏洞利用包（`agent_output/` 下的目录）；
- **并发处理**：通过 `asyncio` + `Semaphore` 控制并发度，提升整体处理速度；
- **统计与日志**：在终端中打印每个漏洞的处理进度、结果以及整体统计信息。

它可以理解为“第一阶段流水线”：**从 CSV 漏洞报告 → 自动分析 → 自动生成 PoC/Exploit 输出目录**。

---

### 2. 模型与系统提示词

#### 2.1 模型配置

```python
model = OpenAIModel(
    "qwen3-coder-480b-a35b-instruct",
    provider=OpenAIProvider(
        api_key=os.getenv('OPEN_API_KEY'),
        base_url="https://chatbox.isrc.ac.cn/api",
    ),
)
```

- **模型类型**：通过 `OpenAIModel` + `OpenAIProvider` 进行封装；
- **密钥来源**：从环境变量 `OPEN_API_KEY` 读取；
- **Base URL**：`https://chatbox.isrc.ac.cn/api`；
- 代码中保留了其他模型配置的注释，可以根据需要切换。

> 使用前请确保 `.env` 中已配置 `OPEN_API_KEY`，并在运行前调用了 `load_dotenv()`（文件中已调用）。

#### 2.2 系统提示词 `SYSTEM_PROMPT`

```python
def load_system_prompt(file_path: str = "system_prompt.txt") -> str:
    ...

SYSTEM_PROMPT = load_system_prompt()
```

- **来源文件**：默认从项目根目录的 `system_prompt.txt` 加载；
- **错误处理**：
  - 若文件不存在：打印警告并返回空字符串；
  - 若读取出错：打印错误并返回空字符串。
- **作用**：作为 `Agent` 的 `system_prompt`，定义了 Agent 行为规范、工作流和输出要求。

> 若修改系统指令，只需编辑 `system_prompt.txt`，无需改动代码本身。

---

### 3. Agent 与工具绑定

#### 3.1 共享 Agent 实例 `agent`

```python
agent = Agent(
    model,
    system_prompt=SYSTEM_PROMPT,
    tools=[
        tools.find_related_files,
        tools.read_file_content,
        tools.read_rtl_lines,
        tools.create_exploit_package,
    ],
)
```

- 虽然定义了一个全局 `agent`，但实际批处理时每个漏洞会使用 `create_fresh_agent()` 创建独立实例（避免对话历史串扰）。
- 绑定的工具来自 `tools.py`：
  - **`find_related_files`**：按模块名查找 DIF/base/tests 等文件；
  - **`read_file_content`**：读取 C/DIF/base 等源码；
  - **`read_rtl_lines`**：按行号规格读取 RTL 源码；
  - **`create_exploit_package`**：在 `agent_output/` 下生成利用包目录及文件。

#### 3.2 每漏洞独立 Agent：`create_fresh_agent()`

```python
def create_fresh_agent():
    """为每个漏洞创建独立的Agent实例(全新会话,无历史上下文)"""
    return Agent(
        model,
        system_prompt=SYSTEM_PROMPT,
        tools=[
            tools.find_related_files,
            tools.read_file_content,
            tools.read_rtl_lines,
            tools.create_exploit_package,
        ],
    )
```

- **目的**：保证每个漏洞分析会话之间**没有历史对话干扰**，各自独立；
- **好处**：
  - 减少上下文长度压力；
  - 防止前一个漏洞的“思路”影响后一个。

---

### 4. 单个漏洞处理逻辑：`process_single_vulnerability`

```python
async def process_single_vulnerability(vuln, idx, total_vulns, semaphore, log_lock, stats_lock, stats):
    ...
```

#### 4.1 输入参数

- **`vuln`**：单条漏洞记录字典，来自 `read_vulnerability_csv`：
  - `vuln['vuln_id']`
  - `vuln['module']`
  - `vuln['filepath']`
  - `vuln['line']`
  - `vuln['finding']`
  - `vuln['security_impact']`
- **`idx` / `total_vulns`**：当前编号和总数，用于打印进度；
- **`semaphore`**：`asyncio.Semaphore`，控制最大并发数；
- **`log_lock` / `stats_lock`**：`asyncio.Lock`，分别用于串行化输出日志和更新统计；
- **`stats`**：统计字典，包含 `success_count`、`failed_count`、`total_time` 等。

#### 4.2 核心步骤

1. **并发控制**：
   ```python
   async with semaphore:
       ...
   ```
2. **初始化工具调用日志**：
   ```python
   tools.tool_usage_log.set([])
   ```
   - 每个漏洞一个独立的工具调用日志列表，最终会写入对应利用包目录中的 `tool_usage.log`。
3. **打印基本信息**：在锁保护下输出当前处理的漏洞 ID、模块、文件、行号等。
4. **创建独立 Agent 实例**：
   ```python
   fresh_agent = create_fresh_agent()
   ```
5. **构造结构化用户 Prompt**：
   - 把漏洞 ID、模块名、RTL 路径、行号、Finding、安全影响等拼成一个清晰的 Markdown 文本；
   - 明确要求 Agent 按“三阶段工作流”处理：
     1. 用 `read_rtl_lines` 看 RTL；
     2. 查 DIF/testutils/tests；
     3. 用 `create_exploit_package` 生成完整利用包。
6. **运行 Agent**：
   ```python
   resp = await fresh_agent.run(user_prompt)
   ```
7. **记录耗时与统计**：
   - 在日志锁下打印输出与耗时；
   - 在统计锁下更新成功数量与总时间。

#### 4.3 错误处理

- 若 Agent 调用过程中抛出异常：
  - 捕获异常与堆栈（`traceback.format_exc()`）；
  - 在日志中打印简化后的错误信息和耗时；
  - 更新 `stats['failed_count']` 与总时间；
  - 返回 `(vuln_id, 'error', error_msg)`。

---

### 5. 并发批处理入口：`process_vulnerabilities_concurrent`

```python
async def process_vulnerabilities_concurrent(vulnerabilities, concurrency=5):
    ...
```

#### 5.1 功能

- 负责在给定并发度下，异步处理漏洞列表 `vulnerabilities`；
- 控制台打印整体 Pipeline 启动信息和最终统计。

#### 5.2 主要逻辑

1. **初始化统计信息 `stats`**；
2. **创建信号量 `Semaphore(concurrency)` 和两个 `asyncio.Lock`**；
3. **构建任务列表**：对每个漏洞调用 `process_single_vulnerability`；
4. **使用 `asyncio.as_completed` 逐个等待任务完成**，实时收集结果；
5. **完成后计算总耗时并打印汇总**：
   - 总处理数量、成功/失败数量、总耗时、平均耗时等；
6. 返回结果列表，每个元素为 `(vuln_id, status, error_msg)`。

---

### 6. 命令行入口：`main(concurrency=5)`

```python
def main(concurrency=5):
    ...

if __name__ == "__main__":
    main()
```

#### 6.1 CSV 加载

- 默认从 `vuln_report/vuln.csv` 加载漏洞列表：
  ```python
  csv_path = "vuln_report/vuln.csv"
  vulnerabilities = tools.read_vulnerability_csv(csv_path)
  ```
- 若列表为空，则打印错误并退出：`❌ No vulnerabilities found in vuln_report/vuln.csv`。

#### 6.2 并发度校验

- 若 `concurrency <= 0`：重置为默认 5；
- 若 `concurrency > 50`：上限截断为 50；
- 在当前脚本中，命令行直接运行会使用默认值 5（如需修改，只能改代码或从其他脚本调用 `main(concurrency=...)`）。

#### 6.3 运行并发处理

- 使用 `asyncio.run(process_vulnerabilities_concurrent(...))` 启动异步任务；
- 捕获 `KeyboardInterrupt` 友好退出；
- 捕获其他异常并打印堆栈，便于调试。

---

### 7. 使用方法与前置条件

#### 7.1 运行前准备

- **环境变量与依赖**：
  - `.env` 中配置 `OPEN_API_KEY`；
  - 安装好依赖（`pydantic_ai`、`python-dotenv` 等）。
- **文件布局**：
  - 项目根目录下存在：
    - `system_prompt.txt`（Agent 的系统指令）；
    - `vuln_report/vuln.csv`（漏洞报告）；
    - `opentitan/` 代码树（`tools.py` 会基于此查找 DIF/base/tests 等）。

#### 7.2 基本运行方式

在项目根目录下执行：

```bash
python agent.py
```

- 默认并发度为 5；
- 执行过程中会在终端输出每个漏洞处理的详细信息和最终汇总统计；
- 生成的漏洞利用包会写入 `agent_output/` 下对应 ID 的子目录（由 `create_exploit_package` 创建）。

> 若想自定义并发度，可在其他 Python 脚本中：
> ```python
> from agent import main
> main(concurrency=10)
> ```

---

### 8. 扩展建议

- **调整工作流**：
  - 可以在构造 `user_prompt` 的部分增加/修改步骤约束，例如加入更多“必须先读哪些文件”的要求；
- **调整工具集**：
  - 可在 `Agent(..., tools=[...])` 中增加新的工具函数（例如额外的搜索/分析工具）；
- **结果后处理**：
  - 当前只在控制台打印 Agent 输出；如有需要，可将 `resp.output` 序列化保存到日志文件或数据库中，方便后续审计与回溯。
