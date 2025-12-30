import os
import sys
import shutil
import re
import subprocess

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 run_agent_test.py <test_id>")
        sys.exit(1)

    test_id = sys.argv[1]
    agent_output_dir = "/home/exp-agent/agent_output"
    opentitan_dir = "/home/exp-agent/opentitan"
    tests_dir = os.path.join(opentitan_dir, "sw/device/tests")
    
    # 1. Find the directory
    target_dir = None
    if os.path.exists(agent_output_dir):
        for d in os.listdir(agent_output_dir):
            if d.startswith(f"{test_id}-"):
                target_dir = os.path.join(agent_output_dir, d)
                break
    
    if not target_dir:
        print(f"Error: Could not find directory for test ID {test_id} in {agent_output_dir}")
        sys.exit(1)
        
    print(f"Found directory: {target_dir}")

    # 2. Read BUILD file
    build_file_path = os.path.join(target_dir, "BUILD")
    if not os.path.exists(build_file_path):
        print(f"Error: BUILD file not found in {target_dir}")
        sys.exit(1)
        
    with open(build_file_path, 'r') as f:
        build_content = f.read()

    # 3. Extract opentitan_test blocks
    start_indices = [m.start() for m in re.finditer(r'opentitan_test\s*\(', build_content)]
    
    target_block = None
    c_file_name = None
    final_test_name = None

    for start in start_indices:
        # Find matching parenthesis to get the full block
        balance = 0
        end = -1
        for i in range(start, len(build_content)):
            if build_content[i] == '(':
                balance += 1
            elif build_content[i] == ')':
                balance -= 1
                if balance == 0:
                    end = i + 1
                    break
        
        if end != -1:
            block = build_content[start:end]
            # Check for srcs. Assuming single file in list for this specific task.
            # Matches: srcs = ["file.c"] or srcs = [ "file.c" ] or srcs = ["file.c",]
            srcs_match = re.search(r'srcs\s*=\s*\[\s*"([^"]+)"(?:,)?\s*\]', block)
            if srcs_match:
                src_file = srcs_match.group(1)
                if src_file.endswith(".c"):
                    c_file_name = src_file
                    
                    # Check name
                    name_match = re.search(r'name\s*=\s*"([^"]+)"', block)
                    if name_match:
                        current_name = name_match.group(1)
                        expected_name = src_file[:-2] # remove .c
                        
                        final_test_name = expected_name
                        
                        if current_name != expected_name:
                            print(f"Renaming test from '{current_name}' to '{expected_name}'")
                            # Replace name field carefully
                            # We use the captured group to ensure we replace the right thing
                            block = block.replace(f'name = "{current_name}"', f'name = "{expected_name}"')
                        
                        target_block = block
                        break
    
    if not target_block or not c_file_name:
        print("Error: Could not find valid opentitan_test block with .c source in BUILD file")
        sys.exit(1)

    # 4. Copy .c file
    src_c_path = os.path.join(target_dir, c_file_name)
    dst_c_path = os.path.join(tests_dir, c_file_name)
    
    if not os.path.exists(src_c_path):
         print(f"Error: Source file {src_c_path} does not exist")
         sys.exit(1)
         
    print(f"Copying {src_c_path} to {dst_c_path}")
    shutil.copy2(src_c_path, dst_c_path)

    # 5. Append to destination BUILD
    dst_build_path = os.path.join(tests_dir, "BUILD")
    print(f"Appending test block to {dst_build_path}")
    
    with open(dst_build_path, 'a') as f:
        f.write("\n" + target_block + "\n")

    # 6. Run Bazel
    cmd = [
        "bazel", "test", 
        "--test_output=streamed", 
        "-j", "16", 
        "--test_timeout=9999", 
        f"//sw/device/tests:{final_test_name}_sim_verilator"
    ]
    
    print(f"Running command: {' '.join(cmd)}")
    print(f"In directory: {opentitan_dir}")
    
    # Flush stdout before running subprocess to ensure order of logs
    sys.stdout.flush()
    
    try:
        subprocess.run(cmd, cwd=opentitan_dir, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Bazel test failed with exit code {e.returncode}")
        sys.exit(e.returncode)

if __name__ == "__main__":
    main()
