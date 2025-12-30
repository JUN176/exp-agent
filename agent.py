from pydantic_ai.models.openai import OpenAIModel
from pydantic_ai.providers.openai import OpenAIProvider
from pydantic_ai import Agent
from dotenv import load_dotenv
import tools
import os
import asyncio
from asyncio import Semaphore
from datetime import datetime
import json

load_dotenv()

# model = OpenAIModel(
# "deepseek-chat",
# provider=OpenAIProvider(api_key=os.getenv('OPENAI_API_KEY'),
#                         base_url="https://api.deepseek.com")
# )
model = OpenAIModel(
    "qwen3-coder-480b-a35b-instruct",
    provider=OpenAIProvider(api_key=os.getenv('OPEN_API_KEY'),
                            base_url="https://chatbox.isrc.ac.cn/api")
)
# model = OpenAIModel(
# "openai/gpt-5",
# provider=OpenAIProvider(api_key=os.getenv('OPEN_ROUTER_API_KEY'),
#                         base_url="https://openrouter.ai/api/v1")
# )


def load_system_prompt(file_path: str = "system_prompt.txt") -> str:
    """从文件加载系统提示词"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"警告: 未找到系统提示词文件 {file_path}")
        return ""
    except Exception as e:
        print(f"错误: 读取系统提示词文件失败: {e}")
        return ""


SYSTEM_PROMPT = load_system_prompt()

# 创建Agent实例
agent = Agent(
    model,
    system_prompt=SYSTEM_PROMPT,
    tools=[tools.find_related_files, tools.read_file_content,
           tools.read_rtl_lines, tools.create_exploit_package]
)


def create_fresh_agent():
    """为每个漏洞创建独立的Agent实例(全新会话,无历史上下文)"""
    return Agent(
        model,
        system_prompt=SYSTEM_PROMPT,
        tools=[tools.find_related_files, tools.read_file_content,
               tools.read_rtl_lines, tools.create_exploit_package]
    )


async def process_single_vulnerability(vuln, idx, total_vulns, semaphore, log_lock, stats_lock, stats):
    """异步处理单个漏洞 - 每个漏洞使用独立Agent实例(无上下文累积)

    Args:
        vuln: 漏洞信息字典
        idx: 当前漏洞索引
        total_vulns: 总漏洞数
        semaphore: 信号量控制并发
        log_lock: 日志锁
        stats_lock: 统计信息锁
        stats: 统计信息字典
    """
    async with semaphore:
        # Initialize tool usage log for this task
        tools.tool_usage_log.set([])

        start_time = datetime.now()

        # 输出开始处理的信息
        async with log_lock:
            print(f"\n{'─'*60}")
            print(
                f"[{idx}/{total_vulns}] Processing Vulnerability #{vuln['vuln_id']}")
            print(f"Module: {vuln['module']}")
            print(f"File: {vuln['filepath']}")
            print(f"Line: {vuln['line']}")
            print(f"{'─'*60}\n")

        # 为每个漏洞创建全新的Agent实例(独立对话上下文)
        fresh_agent = create_fresh_agent()

        # 构建结构化的输入prompt
        user_prompt = f"""
**Vulnerability Analysis Request**

**Vulnerability ID:** {vuln['vuln_id']}
**Module Name:** {vuln['module']}
**RTL File Path:** {vuln['filepath']}
**Vulnerable Lines:** {vuln['line']}

**Finding:**
{vuln['finding']}

**Security Impact:**
{vuln['security_impact']}

**Instructions:**
Please analyze this vulnerability following the standard three-phase workflow:
1. Read the RTL code at the specified lines using the read_rtl_lines tool
2. Gather software context (DIF, testutils, existing tests)
3. Generate exploit package (BUILD + C code + README)

Use the read_rtl_lines tool to examine the specific lines of code mentioned in the vulnerability report. The line specification may contain comma-separated individual lines and dash-separated ranges (e.g., "729,770,811,852,1710-1724").

Generate the complete exploit package and save it using create_exploit_package().
"""

        try:
            # 在独立的对话上下文中运行
            resp = await fresh_agent.run(user_prompt)
            elapsed = (datetime.now() - start_time).total_seconds()

            async with log_lock:
                print(
                    f"\n✓ Agent Response for Vulnerability #{vuln['vuln_id']}:\n{resp.output}\n")
                print(f"⏱️  Completed in {elapsed:.2f}s\n")

            async with stats_lock:
                stats['success_count'] += 1
                stats['total_time'] += elapsed

            return (vuln['vuln_id'], 'success', None)

        except Exception as e:
            elapsed = (datetime.now() - start_time).total_seconds()
            # 获取详细的错误信息
            import traceback
            error_detail = traceback.format_exc()
            error_msg = str(e)[:100]  # 限制错误信息长度

            async with log_lock:
                print(
                    f"\n✗ Error processing vulnerability #{vuln['vuln_id']}: {error_msg}\n")
                print(f"⏱️  Failed after {elapsed:.2f}s\n")
                # 可选：将详细错误信息写入日志文件
                # print(f"Details: {error_detail[:200]}...")

            async with stats_lock:
                stats['failed_count'] += 1
                stats['total_time'] += elapsed

            return (vuln['vuln_id'], 'error', error_msg)


async def process_vulnerabilities_concurrent(vulnerabilities, concurrency=5):
    """并发处理漏洞,支持异步并发

    Args:
        vulnerabilities: 漏洞列表
        concurrency: 最大并发数
    """
    total_vulns = len(vulnerabilities)
    print(f"\n{'='*60}")
    print(f"🚀 Auto Exploit Generation Pipeline Started (Concurrent Mode)")
    print(f"{'='*60}")
    print(f"Total vulnerabilities to process: {total_vulns}")
    print(f"Concurrency level: {concurrency}\n")

    # 统计信息
    stats = {
        'success_count': 0,
        'failed_count': 0,
        'total_time': 0.0
    }

    # 创建信号量控制并发数
    semaphore = Semaphore(concurrency)
    log_lock = asyncio.Lock()
    stats_lock = asyncio.Lock()

    # 创建任务列表
    tasks = [
        process_single_vulnerability(
            vuln, idx, total_vulns, semaphore, log_lock, stats_lock, stats)
        for idx, vuln in enumerate(vulnerabilities, 1)
    ]

    # 记录开始时间
    start_time = datetime.now()

    # 等待所有任务完成
    results = []
    for coro in asyncio.as_completed(tasks):
        result = await coro
        results.append(result)

    # 最终统计
    total_elapsed = (datetime.now() - start_time).total_seconds()
    print(f"\n{'='*60}")
    print(f"📊 Pipeline Execution Summary")
    print(f"{'='*60}")
    print(f"Total Processed: {total_vulns}")
    print(f"✓ Successful: {stats['success_count']}")
    print(f"✗ Failed: {stats['failed_count']}")
    print(f"⏱️  Total Time: {total_elapsed:.2f}s")
    if total_vulns > 0:
        print(
            f"⚡ Average Time per Vulnerability: {stats['total_time']/total_vulns:.2f}s")
    print(f"{'='*60}\n")

    return results


def main(concurrency=5):
    """主函数，支持并发处理漏洞

    Args:
        concurrency (int): 最大并发数，默认为5
    """
    # 批量处理模式
    csv_path = "vuln_report/vuln.csv"
    vulnerabilities = tools.read_vulnerability_csv(csv_path)

    if not vulnerabilities:
        print(f"❌ No vulnerabilities found in {csv_path}")
        return

    # 参数验证
    if concurrency <= 0:
        print("⚠️  Invalid concurrency value, using default value of 5")
        concurrency = 5
    elif concurrency > 50:
        print("⚠️  Concurrency value too high, capping at 50")
        concurrency = 50

    print(f"🔧 Running with concurrency level: {concurrency}")

    # 运行并发处理
    try:
        asyncio.run(process_vulnerabilities_concurrent(
            vulnerabilities, concurrency))
    except KeyboardInterrupt:
        print("\n\n👋 Pipeline interrupted by user. Goodbye!")
    except Exception as e:
        print(f"\n\n💥 Unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
