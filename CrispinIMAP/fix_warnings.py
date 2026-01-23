#!/usr/bin/env python3
"""
Script to fix assignment-in-conditional warnings by adding extra parentheses.
This transforms: if (x = y) into: if ((x = y))
"""

import re
import sys

def fix_file(filename):
    """Fix assignment-in-conditional patterns in a file"""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Patterns to fix - add extra parentheses around assignments in conditionals
    # Pattern: if (var = expr) -> if ((var = expr))
    # Pattern: while (var = expr) -> while ((var = expr))
    
    # More specific patterns to avoid false positives
    patterns = [
        # if (var = expr) but not if ((var = expr))
        (r'\bif\s+\(([a-zA-Z_][a-zA-Z0-9_]*\s*=\s*[^=][^)]*)\)(?!\))', r'if ((\1))'),
        # while (var = expr) but not while ((var = expr))
        (r'\bwhile\s+\(([a-zA-Z_][a-zA-Z0-9_]*\s*=\s*[^=][^)]*)\)(?!\))', r'while ((\1))'),
    ]
    
    original = content
    for pattern, replacement in patterns:
        content = re.sub(pattern, replacement, content)
    
    if content != original:
        with open(filename, 'w') as f:
            f.write(content)
        return True
    return False

if __name__ == '__main__':
    files = ['imap4r1.c', 'netmsg.c', 'auth_log.c']
    
    for filename in files:
        try:
            if fix_file(filename):
                print(f"Fixed: {filename}")
            else:
                print(f"No changes: {filename}")
        except FileNotFoundError:
            print(f"Not found: {filename}")
        except Exception as e:
            print(f"Error processing {filename}: {e}")
