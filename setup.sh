#!/bin/bash

# directories
PROJECT=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# .clang-format
echo "- use linux clang-format"
cd "code"
curl -o .clang-format https://raw.githubusercontent.com/torvalds/linux/dd81e1c7d5fb126e5fbc5c9e334d7b3ec29a16a0/.clang-format
cd ${PROJECT}