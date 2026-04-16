#!/usr/bin/env python3
import subprocess
import os
import sys

def run_command(cmd):
    """Выполнить команду и вернуть результат"""
    print(f"Running: {cmd}")
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result

def main():
    # Очистка
    print("Cleaning project...")
    run_command("make clean")
    
    # Сборка проекта
    print("\nBuilding project...")
    build_result = run_command("make")
    if build_result.returncode != 0:
        print("Build failed!")
        print(build_result.stderr)
        sys.exit(1)
    print("Build successful!")
    
    # Запуск валидных тестов
    print("\n=== Running VALID tests ===")
    test_result = run_command("./test_runner_final")
    print(test_result.stdout)
    
    if test_result.returncode != 0:
        print("Valid tests failed!")
        sys.exit(1)
    
    # Запуск тестов с ошибками
    print("\n=== Running ERROR tests ===")
    error_result = run_command("./test_errors")
    print(error_result.stdout)
    
    if error_result.returncode != 0:
        print("Error tests failed!")
        sys.exit(1)
    
    # Тестирование примера с препроцессором
    print("\n=== Testing preprocessor ===")
    preprocess_result = run_command("./minicompiler preprocess examples/preprocess_test.src")
    print(preprocess_result.stdout)
    
    # Тестирование компиляции
    print("\n=== Testing compilation ===")
    compile_result = run_command("./minicompiler compile examples/hello.src")
    
    print("\n✅ All tests passed!")

if __name__ == "__main__":
    main()