import os
import re
from typing import List, Literal, Optional

def _search_and_match(search_path: str, regex_pattern: str) -> List[str]:
    """
    An internal helper function that performs a regular expression search at the specified path.
    """
    if not os.path.isdir(search_path):
        return []

    try:
        pattern = re.compile(regex_pattern)
    except re.error as e:
        print(f"Error: Invalid regular expression pattern: {e}")
        return []

    matching_files = []
    for root, _, filenames in os.walk(search_path):
        for filename in filenames:
            if pattern.match(filename):
                full_path = os.path.join(root, filename)
                normalized_path = full_path.replace(os.sep, '/')
                matching_files.append(normalized_path)
    
    return matching_files


def find_related_files(
    module_name: str, 
    file_type: Literal['dif', 'testutils', 'tests', 'build', 'all']
) -> List[str]:
    """
    Find all relevant OpenTitan software files by module name and file type.

    This tool encapsulates the file structure and naming conventions of the OpenTitan   repository, simplifying the process of finding context files for Agent

    Args:
        module_name (str): The name of the target IP module (e.g. "otp_ctrl").
        file_type (Literal): The type of file to look for. Can be:
            - 'dif': Find Driver Interface files.
            - 'testutils': Finds the testutils library files.
            - 'tests': Find existing test files for the module.
            - 'build': Find the BUILD file in the test directory.
            - 'all': performs all the above searches and returns a combined list.

    Returns:
        List[str]: a list of strings containing the relative paths of all matching files.Returns an empty list if the file type is invalid or the file was not found.
    """
    search_path = ""
    regex_pattern = ""

    if file_type == 'dif':
        search_path = "opentitan/sw/device/lib/dif"
        regex_pattern = rf"^dif_{module_name}.*\.(h|c)$"
    elif file_type == 'testutils':
        search_path = "opentitan/sw/device/lib/testing"
        regex_pattern = rf"^{module_name}_testutils.*\.(h|c)$"
    elif file_type == 'tests':
        search_path = "opentitan/sw/device/tests"
        regex_pattern = rf"^{module_name}.*\.(c|h)$"
    elif file_type == 'build':
        search_path = "opentitan/sw/device/tests"
        regex_pattern = r"^BUILD$"
    elif file_type == 'all':
        # 递归调用自身以收集所有类型的文件
        all_files = []
        for f_type in ['dif', 'testutils', 'tests', 'build']:
            all_files.extend(find_related_files(module_name, f_type))
        return list(set(all_files)) # 使用 set 去除可能的重复项
    else:
        print(f"Error: Invalid file_type '{file_type}'. "
              f"Must be one of: 'dif', 'testutils', 'tests', 'build', 'all'.")
        return []

    return _search_and_match(search_path, regex_pattern)

def read_file_content(
    file_path: str,
    start_line: Optional[int] = None,
    end_line: Optional[int] = None
) -> str:
    """
    Reads all or part of a given file path.

    This tool is central to the Agent's ability to understand the contents of a file. It can read the entire file to learn the API,or read only specific lines to analyse the exploit code.

    Args:
        file_path (str): relative path to the file (e.g. "sw/device/lib/dif/dif_otp_ctrl.h"). 
        This path is usually provided by the find_related_files utility.
        start_line (Optional[int]): the start line number to read (start at 1 and include this line).
        If None, then read from the beginning of the file.
        end_line (Optional[int]): The end line number to read (from 1, including this line). 
        If None, read to the end of the file.

    Returns:
        str: A single string containing the contents of the requested file. If an error occurs (e.g. file not found,line number not valid), a string describing the error is returned.
    """
    # 1. 输入验证
    if not os.path.isfile(file_path):
        return f"Error: File not found or path is a directory: '{file_path}'"

    if start_line is not None and start_line <= 0:
        return f"Error: start_line must be a positive number, but got {start_line}."
    
    if end_line is not None and end_line <= 0:
        return f"Error: end_line must be a positive number, but got {end_line}."

    if start_line and end_line and start_line > end_line:
        return (f"Error: start_line ({start_line}) cannot be greater than "
                f"end_line ({end_line}).")

    # 2. 文件读取逻辑
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            # Case A: 读取整个文件
            if start_line is None and end_line is None:
                return f.read()

            # Case B: 读取特定行
            lines = f.readlines()
            
            # 转换 1-indexed 行号为 0-indexed 列表索引
            start_index = (start_line - 1) if start_line is not None else 0
            # 如果 end_line 未指定，则读取到最后
            end_index = end_line if end_line is not None else len(lines)
            
            if start_index >= len(lines):
                return (f"Error: start_line ({start_line}) is beyond the end of "
                        f"the file (total lines: {len(lines)}).")
            
            # 使用列表切片获取所需的行
            requested_lines = lines[start_index:end_index]
            return "".join(requested_lines)

    except IOError as e:
        return f"Error: Could not read file '{file_path}'. Reason: {e}"
    
def write_file(file_path: str, content: str) -> str:
    """Writes content to a file, creating directories if needed."""
    try:
        os.makedirs(os.path.dirname(file_path), exist_ok=True)
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        return f"Successfully wrote content to {file_path}"
    except Exception as e:
        return f"Error writing to file {file_path}: {e}"