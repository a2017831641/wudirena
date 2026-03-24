#!/bin/bash

normalize_for_compare() {
    local input_file=$1
    local normalized_file=$2
    perl -0pe 's/\r\n/\n/g; s/\r/\n/g; s/(?<!\n)\z/\n/s' "$input_file" > "$normalized_file"
}

if [ ! -d "./cpp/build" ]; then
    mkdir ./cpp/build
else
    rm -rf ./cpp/build/*
fi

cd ./cpp/build/
# 默认为Release 后附参数Debug可以用于溢出调试
BUILD_TYPE="Release"
if [ $# -ge 1 ]; then
    BUILD_TYPE=$1
fi
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j10
cd ..

if [ -f "../testcase/0.txt" ]; then
    echo -e "\033[1m\033[32m\nUsing test case: 0.txt\033[0m"
    start_time=$(date +%s%N)
    output_file=$(mktemp)
    ./build/MyProject < ../testcase/0.txt > "$output_file"
    program_status=$?
    end_time=$(date +%s%N)
    elapsed_time=$((end_time - start_time))
    elapsed_time_ms=$(echo "scale=3; $elapsed_time / 1000000" | bc)

    cat "$output_file"
    echo -e "\033[1m\033[34m\nExecution time: ${elapsed_time_ms} ms\n\033[0m"

    if [ $program_status -eq 0 ]; then
        answer_file="../answer/0.txt"
        if [ -f "$answer_file" ]; then
            expected_normalized=$(mktemp)
            output_normalized=$(mktemp)
            diff_file=$(mktemp)
            normalize_for_compare "$answer_file" "$expected_normalized"
            normalize_for_compare "$output_file" "$output_normalized"

            if diff -u "$expected_normalized" "$output_normalized" > "$diff_file"; then
                echo -e "\033[1m\033[32mAccepted\033[0m"
            else
                echo -e "\033[1m\033[31mWrong Answer\033[0m"
                echo -e "\033[33mDiff with expected output:\033[0m"
                cat "$diff_file"
            fi

            rm -f "$expected_normalized" "$output_normalized" "$diff_file"
        else
            echo -e "\033[33mNo answer file found: ${answer_file}\033[0m"
        fi
    else
        echo -e "\033[1m\033[31mProgram exited with status ${program_status}\033[0m"
    fi

    rm -f "$output_file"
else
    echo -e "\033[33mPlease input manually\033[0m"
    ./build/MyProject
fi

# 检查是否生成了核心转储文件
CORE_DUMP_FILE=$(ls core* 2>/dev/null | head -n 1)
if [ -n "$CORE_DUMP_FILE" ]; then
    echo -e "\033[1m\033[31mCore dump detected. Analyzing...\n\033[0m"
    gdb -batch -ex "bt" ./build/MyProject "$CORE_DUMP_FILE"
    rm -f "$CORE_DUMP_FILE"
else
    echo -e "\033[32mNo core dump detected.\n\033[0m"
fi
