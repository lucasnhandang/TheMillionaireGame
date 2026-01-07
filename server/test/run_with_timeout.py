#!/usr/bin/env python3
import subprocess
import sys
import os

os.chdir(os.path.dirname(os.path.abspath(__file__)))

try:
    result = subprocess.run(['bash', 'run_all_tests.sh'], 
                          timeout=120, 
                          capture_output=False,
                          text=True)
    sys.exit(result.returncode)
except subprocess.TimeoutExpired:
    print("\n\n❌ TIMEOUT: Tests took longer than 120 seconds - possible infinite loop!")
    sys.exit(1)

