import os
import sys
import re

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 delete.py <test_id>")
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

    # 2. Read source BUILD file to identify the test name and .c file
    src_build_path = os.path.join(target_dir, "BUILD")
    if not os.path.exists(src_build_path):
        print(f"Error: BUILD file not found in {target_dir}")
        sys.exit(1)
        
    with open(src_build_path, 'r') as f:
        src_build_content = f.read()

    # Extract .c file name from source BUILD
    c_file_name = None
    
    # Matches: srcs = ["file.c"] or srcs = [ "file.c" ] or srcs = ["file.c",]
    srcs_match = re.search(r'srcs\s*=\s*\[\s*"([^"]+\.c)"(?:,)?\s*\]', src_build_content)
    if srcs_match:
        c_file_name = srcs_match.group(1)
    
    if not c_file_name:
        print("Error: Could not determine .c file name from source BUILD file")
        sys.exit(1)

    test_name = c_file_name[:-2] # remove .c
    print(f"Target test name: {test_name}")
    print(f"Target C file: {c_file_name}")

    # 3. Remove .c file from destination
    dst_c_path = os.path.join(tests_dir, c_file_name)
    if os.path.exists(dst_c_path):
        print(f"Removing file: {dst_c_path}")
        os.remove(dst_c_path)
    else:
        print(f"File not found (already deleted?): {dst_c_path}")

    # 4. Remove block from destination BUILD file
    dst_build_path = os.path.join(tests_dir, "BUILD")
    if not os.path.exists(dst_build_path):
        print(f"Error: Destination BUILD file not found at {dst_build_path}")
        sys.exit(1)

    with open(dst_build_path, 'r') as f:
        content = f.read()

    # Find blocks to remove
    blocks_to_remove = []
    
    cursor = 0
    while True:
        match = re.search(r'opentitan_test\s*\(', content[cursor:])
        if not match:
            break
        
        start = cursor + match.start()
        
        # Find end of block
        balance = 0
        end = -1
        for i in range(start, len(content)):
            if content[i] == '(':
                balance += 1
            elif content[i] == ')':
                balance -= 1
                if balance == 0:
                    end = i + 1
                    break
        
        if end != -1:
            block = content[start:end]
            # Check if this block has the target name
            name_match = re.search(r'name\s*=\s*"([^"]+)"', block)
            if name_match and name_match.group(1) == test_name:
                blocks_to_remove.append((start, end))
        
        cursor = start + 1 # Move forward
        
    if not blocks_to_remove:
        print(f"Warning: Could not find opentitan_test block for '{test_name}' in {dst_build_path}")
    else:
        # Remove blocks. Sort by start index descending to remove from end first
        blocks_to_remove.sort(key=lambda x: x[0], reverse=True)
        
        new_content = content
        for start, end in blocks_to_remove:
            print(f"Removing block for '{test_name}' at indices {start}-{end}")
            prefix = new_content[:start]
            suffix = new_content[end:]
            new_content = prefix + suffix

        # Write back
        with open(dst_build_path, 'w') as f:
            f.write(new_content)
        print("Updated BUILD file.")

if __name__ == "__main__":
    main()
