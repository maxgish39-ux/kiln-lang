"""Legacy test command retained as a safe alias for the full MINT suite."""
import subprocess
import sys
from pathlib import Path

if __name__ == "__main__":
    tests_dir = Path(__file__).resolve().parent
    result = subprocess.run(
        [sys.executable, "-m", "unittest", "discover", "-s", str(tests_dir), "-p", "test_mint.py", "-v"],
        check=False,
    )
    raise SystemExit(result.returncode)
