#!/bin/bash
set -e
set -o pipefail

if [ $(git clang-format | grep -cE 'clang-format did not modify any files|no modified files to format') -eq 1 ]; then
    echo 'Clang-format check pass'
    exit 0
else
    echo 'Clang-format check failed. Pls review your code change'
    exit 1
fi

