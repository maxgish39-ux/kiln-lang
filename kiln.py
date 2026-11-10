#!/usr/bin/env python3
"""Kiln Programming Language command-line runner."""
import argparse
import sys
import shutil
import subprocess
from pathlib import Path

from kiln import __version__
from kiln.lexer import tokenize
from kiln.parser import Parser
from kiln.interpreter import Interpreter, KilnRuntimeError
from kiln.compiler import CCompiler, NativeCompileError


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] == "build":
        build_parser = argparse.ArgumentParser(prog="kiln1 build", description="Compile a Kiln program to a native executable")
        build_parser.add_argument("file", help="Kiln source file")
        build_parser.add_argument("-o", "--output", help="output executable name")
        build_parser.add_argument("--emit-c", action="store_true", help="emit generated C source instead of compiling")
        build_args = build_parser.parse_args(argv[1:])
        source_path = Path(build_args.file).expanduser().resolve()
        try:
            source = source_path.read_text(encoding="utf-8")
            statements = Parser(tokenize(source)).parse()
            c_source = CCompiler().compile(statements)
            if build_args.emit_c:
                output = Path(build_args.output or source_path.with_suffix(".c")).expanduser()
                output.write_text(c_source, encoding="utf-8")
                print(f"Generated C source: {output}")
                return 0
            compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
            if not compiler:
                print("BuildError: install a C compiler (for example: sudo apt install build-essential)", file=sys.stderr)
                return 1
            output = Path(build_args.output or source_path.with_suffix(""))
            if not output.is_absolute():
                output = Path.cwd() / output
            output.parent.mkdir(parents=True, exist_ok=True)
            c_path = output.with_name(output.name + ".kiln-build.c")
            c_path.write_text(c_source, encoding="utf-8")
            result = subprocess.run([compiler, "-std=c11", "-O2", str(c_path), "-o", str(output)],
                                    capture_output=True, text=True)
            if result.returncode:
                print("NativeCompileError:\n" + result.stderr, file=sys.stderr)
                return result.returncode
            try: c_path.unlink()
            except OSError: pass
            output.chmod(output.stat().st_mode | 0o111)
            print(f"Built native executable: {output}")
            return 0
        except FileNotFoundError:
            print(f"BuildError: file not found: {source_path}", file=sys.stderr)
        except (SyntaxError, NativeCompileError) as exc:
            print(f"BuildError: {exc}", file=sys.stderr)
        except OSError as exc:
            print(f"BuildError: {exc}", file=sys.stderr)
        return 1

    parser = argparse.ArgumentParser(prog="kiln1", description="Run, check and build Kiln programs")
    parser.add_argument("--version", action="version", version=f"Kiln {__version__}")
    parser.add_argument("--check", action="store_true", help="parse a program without executing it")
    parser.add_argument("file", nargs="?", help="path to a .kiln program")
    args = parser.parse_args(argv)

    if not args.file:
        parser.print_help()
        return 0

    path = Path(args.file).expanduser().resolve()
    try:
        source = path.read_text(encoding="utf-8")
        statements = Parser(tokenize(source)).parse()
        if args.check:
            print(f"OK: {path.name} parsed successfully")
            return 0
        Interpreter(source_dir=path.parent).run(statements, source_path=path)
        return 0
    except FileNotFoundError:
        print(f"Error: file not found: {path}", file=sys.stderr)
    except (SyntaxError, KilnRuntimeError) as exc:
        print(f"{type(exc).__name__}: {exc}", file=sys.stderr)
    except OSError as exc:
        print(f"FileError: {exc}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
