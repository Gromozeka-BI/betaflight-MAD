#!/usr/bin/env python3
import collections
import os
import re

root = os.path.join(os.path.dirname(__file__), "..", "src", "config", "configs")
c = collections.Counter()
n = 0
for dirpath, _, files in os.walk(root):
    if "config.h" not in files:
        continue
    n += 1
    text = open(os.path.join(dirpath, "config.h"), encoding="utf-8", errors="ignore").read()
    m = re.search(r"FC_TARGET_MCU\s+(\S+)", text)
    c[m.group(1) if m else "UNKNOWN"] += 1
print("boards", n)
for k, v in c.most_common():
    print(f"{v:4} {k}")
