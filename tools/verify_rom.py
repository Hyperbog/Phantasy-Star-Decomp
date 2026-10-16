#!/usr/bin/env python3
"""Fails closed on wrong edition or ROM mutation before executing native code."""
import hashlib
import sys
from pathlib import Path
EXPECTED='9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88'
if len(sys.argv)!=2:
    raise SystemExit('Usage: python tools/verify_rom.py /path/to/EnglishRev3.sms')
p=Path(sys.argv[1]); actual=hashlib.sha256(p.read_bytes()).hexdigest()
if actual!=EXPECTED:raise SystemExit(f'Wrong ROM: {actual}; expected {EXPECTED}')
print(f'REV3_SHA256_MATCH size={p.stat().st_size} sha256={actual}')
