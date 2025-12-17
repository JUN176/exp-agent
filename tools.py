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
    file_type: Literal['dif', 'testutils', 'tests', 'build', 'base', 'all']
) -> List[str]:
    """
    Find all relevant OpenTitan software files by module name and file type.

    This tool encapsulates the file structure and naming conventions of the OpenTitan   repository, simplifying the process of finding context files for Agent

    Args:
        module_name (str): The name of the target IP module (e.g. "otp_ctrl").
        file_type (Literal): The type of file to look for. Can be:
            - 'base': Find base library files (PRIORITY for POC development).
            - 'dif': Find Driver Interface files (REFERENCE ONLY).
            - 'testutils': Finds the testutils library files.
            - 'tests': Find existing test files for the module.
            - 'build': Find the BUILD file in the test directory.
            - 'all': performs all the above searches and returns a combined list.

    Returns:
        List[str]: a list of strings containing the relative paths of all matching files.Returns an empty list if the file type is invalid or the file was not found.
    """
    search_path = ""
    regex_pattern = ""

    if file_type == 'base':
        search_path = "opentitan/sw/device/lib/base"
        regex_pattern = r"^.*\.(h|c)$"  # All base library files
    elif file_type == 'dif':
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
        for f_type in ['base', 'dif', 'testutils', 'tests', 'build']:
            all_files.extend(find_related_files(module_name, f_type))
        return list(set(all_files))  # 使用 set 去除可能的重复项
    else:
        print(f"Error: Invalid file_type '{file_type}'. "
              f"Must be one of: 'base', 'dif', 'testutils', 'tests', 'build', 'all'.")
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


def read_rtl_lines(file_path: str, line_spec: str) -> str:
    """
    根据行号规格读取RTL文件的指定行。

    这个工具可以解析复杂的行号表示，如"729,770,811,852,1710-1724"，并读取对应的行内容。

    Args:
        file_path (str): RTL文件的相对路径
        line_spec (str): 行号规格，支持逗号分隔的单行号和连字符分隔的行范围，例如"729,770,811,852,1710-1724"

    Returns:
        str: 包含所有指定行内容的字符串，每行前面带有行号
    """
    # 解析行号规格
    line_numbers = set()

    # 分割逗号分隔的部分
    parts = line_spec.split(',')

    for part in parts:
        part = part.strip()
        if '-' in part:
            # 处理范围，如"1710-1724"
            try:
                start, end = map(int, part.split('-'))
                if start <= end:
                    line_numbers.update(range(start, end + 1))
                else:
                    return f"Error: Invalid line range '{part}' (start > end)"
            except ValueError:
                return f"Error: Invalid line range format '{part}'"
        else:
            # 处理单行号
            try:
                line_numbers.add(int(part))
            except ValueError:
                return f"Error: Invalid line number '{part}'"

    # 排序行号
    sorted_lines = sorted(list(line_numbers))

    # 读取文件内容
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            all_lines = f.readlines()
    except IOError as e:
        return f"Error: Could not read file '{file_path}'. Reason: {e}"

    # 提取指定行
    result_lines = []
    for line_num in sorted_lines:
        # 检查行号是否在文件范围内 (1-indexed)
        if 1 <= line_num <= len(all_lines):
            # 添加行号前缀
            result_lines.append(f"L{line_num:04d}: {all_lines[line_num - 1]}")
        else:
            result_lines.append(
                f"L{line_num:04d}: [Line not found in file (file has {len(all_lines)} lines)]\n")

    return ''.join(result_lines)


def read_vulnerability_csv(csv_path: str) -> List[dict]:
    """
    从CSV文件读取漏洞信息列表。

    这个工具是批量处理的入口,它解析CSV文件并返回结构化的漏洞数据,
    每条记录包含完整的漏洞定位和描述信息。

    Args:
        csv_path (str): CSV文件的路径(例如 "vuln_report/vuln.csv")

    Returns:
        List[dict]: 漏洞记录列表,每个字典包含以下字段:
            - vuln_id: 漏洞ID
            - filepath: RTL文件路径
            - module: 模块名称
            - line: 行号信息
            - finding: 发现的问题
            - security_impact: 安全影响

    CSV Schema:
        vuln-id,filepath,moudle,line,Finding,Security impact
    """
    import csv

    vulnerabilities = []

    try:
        # 使用 utf-8-sig 处理BOM, errors='ignore' 跳过无效字符
        with open(csv_path, 'r', encoding='utf-8-sig', errors='replace') as f:
            reader = csv.DictReader(f)
            for row in reader:
                # 跳过空行
                if not row.get('vuln-id') or not row['vuln-id'].strip():
                    continue

                # 处理可能缺失的字段
                def safe_strip(value):
                    return value.strip() if value is not None else ''

                vuln_record = {
                    'vuln_id': safe_strip(row.get('vuln-id', '')),
                    'filepath': safe_strip(row.get('filepath', '')),
                    # 注意这里是moudle而不是module
                    'module': safe_strip(row.get('moudle', '')),
                    'line': safe_strip(row.get('line', '')),
                    'finding': safe_strip(row.get('Finding', '')),
                    'security_impact': safe_strip(row.get('Security impact', ''))
                }
                vulnerabilities.append(vuln_record)

        return vulnerabilities

    except FileNotFoundError:
        print(f"Error: CSV file not found: {csv_path}")
        return []
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        return []


def create_exploit_package(
    vulnerability_name: str,
    c_filename: Optional[str] = None,
    c_content: Optional[str] = None,
    asm_filename: Optional[str] = None,
    asm_content: Optional[str] = None,
    build_content: Optional[str] = None,
    readme_content: Optional[str] = None,
    output_base_dir: str = "agent_output",
    vuln_id: Optional[str] = None
) -> str:
    """
    创建一个完整的漏洞利用包，包含所有必要的文件。

    这个工具是Agent输出的核心接口，它会创建一个标准化的目录结构来存储
    漏洞利用代码、构建配置和说明文档。

    Args:
        vulnerability_name (str): 漏洞名称，用于创建子文件夹（例如 "otp_ctrl_predictor"）
        c_filename (Optional[str]): C文件的文件名（例如 "otp_ctrl_vuln_test.c"）
        c_content (Optional[str]): C代码的完整内容
        asm_filename (Optional[str]): 汇编文件的文件名（例如 "exploit.S"）
        asm_content (Optional[str]): 汇编代码的完整内容
        build_content (Optional[str]): BUILD文件的完整内容
        readme_content (Optional[str]): README.md文件的内容，包含漏洞说明和使用方法
        output_base_dir (str): 输出的基础目录，默认为 "agent_output"
        vuln_id (Optional[str]): 漏洞ID，用于创建带前缀的目录名

    Returns:
        str: 操作结果描述，包括创建的文件列表和路径信息

    Directory Structure:
        agent_output/
        └── {vuln_id}-{vulnerability_name}/ (if vuln_id provided)
        └── {vulnerability_name}/ (if vuln_id not provided)
            ├── BUILD
            ├── {c_filename}.c (if provided)
            ├── {asm_filename}.S (if provided)
            └── README.md (if provided)
    """
    # 创建漏洞专属目录，如果提供了vuln_id则添加前缀
    if vuln_id:
        dir_name = f"{vuln_id}-{vulnerability_name}"
    else:
        dir_name = vulnerability_name
    vuln_dir = os.path.join(output_base_dir, dir_name)

    try:
        os.makedirs(vuln_dir, exist_ok=True)
        created_files = []

        # 写入BUILD文件
        if build_content:
            build_path = os.path.join(vuln_dir, "BUILD")
            with open(build_path, 'w', encoding='utf-8') as f:
                f.write(build_content)
            created_files.append(build_path)

        # 写入C文件
        if c_content and c_filename:
            c_path = os.path.join(vuln_dir, c_filename)
            with open(c_path, 'w', encoding='utf-8') as f:
                f.write(c_content)
            created_files.append(c_path)

        # 写入汇编文件
        if asm_content and asm_filename:
            asm_path = os.path.join(vuln_dir, asm_filename)
            with open(asm_path, 'w', encoding='utf-8') as f:
                f.write(asm_content)
            created_files.append(asm_path)

        # 写入README文件
        if readme_content:
            readme_path = os.path.join(vuln_dir, "README.md")
            with open(readme_path, 'w', encoding='utf-8') as f:
                f.write(readme_content)
            created_files.append(readme_path)

        # 构建成功消息
        result = f"✓ Exploit package created successfully at: {vuln_dir}\n"
        result += f"\nCreated files ({len(created_files)}):"
        for file_path in created_files:
            result += f"\n  - {os.path.basename(file_path)}"

        return result

    except Exception as e:
        return f"Error creating exploit package: {e}"
