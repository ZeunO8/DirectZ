#!/usr/bin/env python3
import sys
import os
import re

def fix_file(path):
    with open(path, 'rb') as f:
        content = f.read()

    fixed_content = re.sub(rb'-Wformat\s+=format-security', b'-Wformat-security', content)

    if fixed_content != content:
        backup_path = path + '.bak'
        if not os.path.exists(backup_path):
            os.rename(path, backup_path)
        with open(path, 'wb') as f:
            f.write(fixed_content)
        print(f'[PATCHED] {path}')

def main(build_dir):
    for root, _, files in os.walk(build_dir):
        for file in files:
            if file.endswith(('.ninja', '.make', '.mk', '.cmake')):
                fix_file(os.path.join(root, file))

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: fix_format_security.py <build_dir>')
        sys.exit(1)
    main(sys.argv[1])