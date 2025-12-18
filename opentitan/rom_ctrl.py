import json
from pathlib import Path
import shutil

buggy_file_db="buggy_file_db.json"
official_file_db="official_file_db.json"

buggy_file_db = json.load(open(buggy_file_db))
official_file_db = json.load(open(official_file_db))

diff_file_list = sorted(list(buggy_file_db.keys()))

target_repo = Path('.')

keep_official_file_list_str="""hw/ip/rom_ctrl/rtl/rom_ctrl.sv"""

keep_official_file_list = keep_official_file_list_str.split('\n')

for file_path in diff_file_list:
    if file_path in keep_official_file_list:
        Path(target_repo / file_path).write_text(official_file_db[file_path])
    else:
        Path(target_repo / file_path).write_text(buggy_file_db[file_path])
print('done')