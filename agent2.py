import os
import shutil
import subprocess
import re
import asyncio
import sys
import threading
import time
from typing import Optional
from pydantic_ai import Agent
from pydantic_ai.models.openai import OpenAIModel
from pydantic_ai.providers.openai import OpenAIProvider
from dotenv import load_dotenv

load_dotenv()

# Configuration
AGENT_OUTPUT_DIR = "/home/exp-agent/agent_output"
OPENTITAN_TESTS_DIR = "/home/exp-agent/opentitan/sw/device/tests"
OPENTITAN_ROOT = "/home/exp-agent/opentitan"
DEST_BUILD_FILE = os.path.join(OPENTITAN_TESTS_DIR, "BUILD")

# Global Lock for Bazel
BAZEL_LOCK = threading.Lock()

# Model Setup
model = OpenAIModel(
    "gpt-5-mini",
    provider=OpenAIProvider(api_key=os.getenv('OPEN_API_KEY'),
                            base_url="https://chatbox.isrc.ac.cn/api")
)

# --- Tools ---

def read_file(file_path: str) -> str:
    """Reads the content of a file."""
    try:
        if not os.path.exists(file_path):
            return f"Error: File not found: {file_path}"
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        return f"Error reading file: {e}"

def write_file(file_path: str, content: str) -> str:
    """Overwrites a file with new content."""
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return f"Successfully wrote to {file_path}"
    except Exception as e:
        return f"Error writing file: {e}"

def run_bazel_test(test_name: str) -> str:
    """Runs the bazel test and returns the output (stdout + stderr)."""
    # Sanitize test_name in case the agent passes a full label or suffix
    if test_name.startswith("//"):
        if ":" in test_name:
            test_name = test_name.split(":")[-1]
            
    if test_name.endswith("_sim_verilator"):
        test_name = test_name.replace("_sim_verilator", "")

    target = f"//sw/device/tests:{test_name}_sim_verilator"
    cmd = [
        "bazel", "test",
        "--test_output=streamed",
        "-j", "16",
        "--test_timeout=9999",
        target
    ]
    
    # Acquire lock to prevent concurrent Bazel executions
    if not BAZEL_LOCK.acquire(blocking=False):
        return "EXECUTION ERROR: Bazel is already running. Please wait for the previous test to finish."
    
    try:
        print(f"🚀 Starting Bazel Test: {target}")
        print(f"📂 Working Directory: {OPENTITAN_ROOT}")
        print(f"💻 Command: {' '.join(cmd)}")
        print("-" * 20 + " BAZEL OUTPUT START " + "-" * 20)
        
        output_buffer = []
        last_line_holder = {"text": ""}
        stop_heartbeat = threading.Event()
        process = None

        def heartbeat():
            while not stop_heartbeat.is_set():
                time.sleep(20)
                if stop_heartbeat.is_set():
                    break
                # Print the last line seen
                msg = last_line_holder["text"].strip()
                if msg:
                    timestamp = time.strftime("%H:%M:%S")
                    print(f"\n⏳ [{timestamp} - Still Running] Last Bazel Output: {msg}")
                    sys.stdout.flush()

        heartbeat_thread = threading.Thread(target=heartbeat, daemon=True)
        heartbeat_thread.start()
        
        try:
            # Use Popen to stream output
            process = subprocess.Popen(
                cmd,
                cwd=OPENTITAN_ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                universal_newlines=True
            )
            
            # Read stdout line by line
            while True:
                line = process.stdout.readline()
                if not line and process.poll() is not None:
                    break
                if line:
                    print(line, end='') # Stream to user terminal
                    sys.stdout.flush()  # Ensure output is displayed immediately
                    output_buffer.append(line) # Capture for LLM
                    last_line_holder["text"] = line
                    
            return_code = process.poll()
            full_output = "".join(output_buffer)
            
            print("-" * 20 + " BAZEL OUTPUT END " + "-" * 20)
            
            status = "PASS" if return_code == 0 else f"FAIL (Exit Code {return_code})"
            output = f"Status: {status}\n\nOutput:\n{full_output}"
            
            # Truncate output if too long to avoid context limit issues
            if len(output) > 50000:
                output = output[:25000] + "\n...[Output Truncated]...\n" + output[-25000:]
            
            return output
        except Exception as e:
            # If an error occurs (e.g. cancellation), ensure we kill the subprocess
            if process and process.poll() is None:
                print(f"\n⚠️ Terminating Bazel process due to error: {e}")
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
            raise e
        finally:
            stop_heartbeat.set()
            heartbeat_thread.join(timeout=1)
                
    except Exception as e:
        return f"EXECUTION ERROR: {e}"
    finally:
        BAZEL_LOCK.release()

def update_build_rule(test_name: str, new_rule_content: str) -> str:
    """Updates the opentitan_test rule for the given test_name in sw/device/tests/BUILD."""
    try:
        with open(DEST_BUILD_FILE, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Regex to find the start of the rule
        pattern = re.compile(rf'opentitan_test\s*\(\s*name\s*=\s*"{test_name}"', re.MULTILINE)
        match = pattern.search(content)
        if not match:
            return f"Error: Rule for {test_name} not found in BUILD file."
        
        start_idx = match.start()
        
        # Find the matching closing parenthesis by counting braces
        open_count = 0
        end_idx = -1
        for i in range(start_idx, len(content)):
            if content[i] == '(':
                open_count += 1
            elif content[i] == ')':
                open_count -= 1
                if open_count == 0:
                    end_idx = i + 1
                    break
        
        if end_idx == -1:
            return "Error: Could not find closing parenthesis for the rule."
            
        # Replace the old rule with the new rule
        new_content = content[:start_idx] + new_rule_content + content[end_idx:]
        
        with open(DEST_BUILD_FILE, 'w', encoding='utf-8') as f:
            f.write(new_content)
            
        return "Successfully updated BUILD rule."
        
    except Exception as e:
        return f"Error updating BUILD rule: {e}"

# --- Agent Definition ---

system_prompt = """
You are an expert Embedded Systems Engineer and Bazel Build Master specializing in OpenTitan.
Your task is to verify and fix C tests for hardware vulnerabilities.

You will be provided with a test name and the paths to the C file and BUILD file.
Your goal is to make the test pass (`bazel test` returns success).

Process:
1.  **Run the test** using `run_bazel_test(test_name)`.
2.  **Analyze the output**:
    *   If the test **PASSES**, return a success message and stop.
    *   If the test **FAILS**:
        *   Read the C file using `read_file`.
        *   Read the BUILD rule (if needed) or infer from context.
        *   Analyze the error (compilation error, linker error, or runtime assertion failure).
        *   **Fix the code**:
            *   Use `write_file` to fix C code errors (syntax, logic, missing headers).
            *   Use `update_build_rule` to fix BUILD errors (missing dependencies, wrong params).
        *   **Retry**: Go back to step 1.

Guidelines:
*   When fixing C code, ensure you keep the core logic of the vulnerability test. Do not just delete the failing check unless it is clearly incorrect.
*   If a dependency is missing in the BUILD file (e.g., header not found), add it to the `deps` list in the `opentitan_test` rule.
*   If the test times out, check if the C code has infinite loops or if the timeout in BUILD is too low.
*   Always read the file content before writing to ensure you don't lose context.
"""

fixer_agent = Agent(
    model,
    system_prompt=system_prompt,
    tools=[read_file, write_file, run_bazel_test, update_build_rule]
)

# --- Main Controller ---

async def process_vulnerability(vuln_dir):
    vuln_path = os.path.join(AGENT_OUTPUT_DIR, vuln_dir)
    print(f"\n{'='*60}")
    print(f"Processing: {vuln_dir}")
    print(f"{'='*60}")
    
    # 1. Identify files
    c_file = None
    build_file = None
    for f in os.listdir(vuln_path):
        if f.endswith(".c"):
            c_file = f
        elif f == "BUILD":
            build_file = f
            
    if not c_file or not build_file:
        print("⚠️  Skipping: Missing .c or BUILD file")
        return

    # 2. Copy C file
    src_c = os.path.join(vuln_path, c_file)
    dst_c = os.path.join(OPENTITAN_TESTS_DIR, c_file)
    try:
        shutil.copy2(src_c, dst_c)
        print(f"✓ Copied {c_file}")
    except Exception as e:
        print(f"❌ Failed to copy C file: {e}")
        return

    # 3. Setup BUILD
    src_build = os.path.join(vuln_path, build_file)
    try:
        with open(src_build, 'r') as f:
            raw_build_content = f.read()
        
        # Extract opentitan_test rule block
        start_match = re.search(r'opentitan_test\s*\(', raw_build_content)
        if not start_match:
             print("⚠️  Skipping: Could not find opentitan_test rule in BUILD")
             return
        
        start_idx = start_match.start()
        
        # Find matching closing parenthesis
        open_count = 0
        end_idx = -1
        for i in range(start_idx, len(raw_build_content)):
            if raw_build_content[i] == '(':
                open_count += 1
            elif raw_build_content[i] == ')':
                open_count -= 1
                if open_count == 0:
                    end_idx = i + 1
                    break
        
        if end_idx == -1:
             print("⚠️  Skipping: Malformed opentitan_test rule")
             return

        rule_content = raw_build_content[start_idx:end_idx]

        # Extract srcs to determine correct name
        srcs_match = re.search(r'srcs\s*=\s*\["([^"]+)"\]', rule_content)
        if not srcs_match:
             print("⚠️  Skipping: Could not find srcs in opentitan_test rule")
             return
        
        c_filename = srcs_match.group(1)
        expected_name = c_filename.replace(".c", "")
        
        # Extract current name
        name_match = re.search(r'name\s*=\s*"([^"]+)"', rule_content)
        if not name_match:
             print("⚠️  Skipping: Could not find name in opentitan_test rule")
             return
        
        current_name = name_match.group(1)
        test_name = expected_name
        
        # Fix name if it doesn't match srcs
        if current_name != expected_name:
            print(f"ℹ️  Fixing test name: '{current_name}' -> '{expected_name}'")
            rule_content = rule_content.replace(f'name = "{current_name}"', f'name = "{expected_name}"')
        
        # Append to BUILD if not exists
        with open(DEST_BUILD_FILE, 'r') as f:
            dest_content = f.read()
        
        if f'name = "{test_name}"' not in dest_content:
            # Append to the end of the file as requested
            new_entry = f"\n\n# Added by agent2.py for {vuln_dir}\n{rule_content}"
            
            print(f"Appending {test_name} to end of BUILD...")
            with open(DEST_BUILD_FILE, 'a') as f:
                f.write(new_entry)
            print(f"✓ Added {test_name} to BUILD")
        else:
            print(f"ℹ️  Test {test_name} already in BUILD")
            
    except Exception as e:
        print(f"❌ Failed to setup BUILD: {e}")
        return

    # 4. Invoke Agent to Verify and Fix
    print(f"🤖 Invoking Agent to verify/fix {test_name}...")
    
    prompt = f"""
    I have set up a test named '{test_name}'.
    
    The C source file is located at: {dst_c}
    The BUILD file is located at: {DEST_BUILD_FILE}
    
    Please run the test. If it fails, analyze the error and fix the C code or the BUILD rule until it passes.
    """
    
    try:
        # Run the agent
        result = await fixer_agent.run(prompt)
        print(f"\nAgent Report for {test_name}:\n{result.data}")
        
    except Exception as e:
        print(f"❌ Agent failed to process {test_name}: {e}")

async def main():
    if not os.path.exists(AGENT_OUTPUT_DIR):
        print(f"❌ Error: Agent output directory not found: {AGENT_OUTPUT_DIR}")
        return
        
    if not os.path.exists(OPENTITAN_TESTS_DIR):
        print(f"❌ Error: OpenTitan tests directory not found: {OPENTITAN_TESTS_DIR}")
        return

    # Backup BUILD file
    if not os.path.exists(DEST_BUILD_FILE + ".bak"):
        shutil.copy2(DEST_BUILD_FILE, DEST_BUILD_FILE + ".bak")
        print(f"ℹ️  Backed up BUILD file to {DEST_BUILD_FILE}.bak")

    # Get all subdirectories
    all_vuln_dirs = sorted([d for d in os.listdir(AGENT_OUTPUT_DIR) 
                       if os.path.isdir(os.path.join(AGENT_OUTPUT_DIR, d))])
    
    if not all_vuln_dirs:
        print("No vulnerability directories found.")
        return

    # Filter based on command line argument
    vuln_dirs = []
    if len(sys.argv) > 1:
        target_id = sys.argv[1]
        print(f"Targeting vulnerability ID: {target_id}")
        
        # Filter directories starting with "{target_id}-"
        vuln_dirs = [d for d in all_vuln_dirs if d.startswith(f"{target_id}-") or d == target_id]
        
        if not vuln_dirs:
            print(f"❌ No directory found for ID {target_id}")
            return
    else:
        vuln_dirs = all_vuln_dirs

    print(f"Found {len(vuln_dirs)} vulnerabilities to process.")
    
    for vuln_dir in vuln_dirs:
        await process_vulnerability(vuln_dir)

if __name__ == "__main__":
    asyncio.run(main())
