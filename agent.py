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
