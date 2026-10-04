"""Run the same read-only quality gates locally and in CI."""

import argparse
import concurrent.futures
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(command):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        print("$ " + " ".join(map(str, command)))
        print(result.stdout + result.stderr)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build/dev")
    parser.add_argument(
        "--static", action="store_true", help="Only run native analysis"
    )
    parser.add_argument("--fast", action="store_true", help="Skip native analysis")
    args = parser.parse_args()
    # Explicit scopes also work in source distributions, without Git metadata.
    native = [
        p
        for directory in ("src", "tests")
        for p in (ROOT / directory).rglob("*")
        if p.suffix in (".c", ".h") and not p.name.startswith("tcp")
    ]
    scripts = [ROOT / "build.sh", *sorted((ROOT / "experiments").rglob("*.sh"))]
    cmake = [ROOT / "CMakeLists.txt", *sorted((ROOT / "cmake").glob("*.cmake"))]
    commands = []
    if not args.static:
        commands.extend(
            [
                ["ruff", "check", "horse", "experiments", "tests", "tools"],
                ["ruff", "format", "--check", "horse", "experiments", "tests", "tools"],
                ["cython-lint", "horse"],
                ["clang-format", "--dry-run", "--Werror", *map(str, native)],
                ["cmake-format", "--check", *map(str, cmake)],
                [
                    "cmake-lint",
                    "-c",
                    str(ROOT / ".cmake-format.yaml"),
                    *map(str, cmake),
                ],
                ["shellcheck", *map(str, scripts)],
                ["shfmt", "-d", "-i", "4", *map(str, scripts)],
                ["actionlint"],
            ]
        )
    failures = sum(bool(run(command)) for command in commands)
    if not args.fast:
        database = ROOT / args.build_dir / "compile_commands.json"
        if not database.exists():
            parser.error(f"Configure CMake first: {database} is missing")
        sources = {str(p) for p in native if p.suffix == ".c"}
        commands = [
            ["clang-tidy", "--quiet", "-p", str(database.parent), entry["file"]]
            for entry in json.loads(database.read_text())
            if entry["file"] in sources
        ]
        if not commands:
            parser.error("Compilation database contains no maintained C sources")
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            failures += sum(bool(status) for status in pool.map(run, commands))
    if not failures:
        print("All quality checks passed")
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
