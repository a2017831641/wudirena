#!/bin/bash

normalize_for_compare() {
    local input_file=$1
    local normalized_file=$2
    perl -0pe 's/\r\n/\n/g; s/\r/\n/g; s/(?<!\n)\z/\n/s' "$input_file" > "$normalized_file"
}

if [ ! -f "./cpp/build/MyProject" ]; then
    echo "Executable ./cpp/build/MyProject not found. Please run bash make.sh first."
    exit 1
fi

if [ -d "./testcase" ]; then
    all_case_time=0
    passed_count=0
    failed_count=0
    missing_answer_count=0

    for testcase in ./testcase/*.txt; do
        if [ -f "$testcase" ]; then
            testcase_name=$(basename "$testcase")
            answer_file="./answer/${testcase_name}"
            output_file=$(mktemp)

            echo -e "\033[1m\033[32m\nUsing test case: $testcase_name\033[0m"
            case_start=$(date +%s%N)
            ./cpp/build/MyProject < "$testcase" > "$output_file"
            program_status=$?
            case_end=$(date +%s%N)

            cat "$output_file"

            elapsed_time=$((case_end - case_start))
            all_case_time=$((all_case_time + elapsed_time))
            elapsed_time_ms=$(echo "scale=3; $elapsed_time / 1000000" | bc)
            echo -e "\033[34mExecution time for ${testcase_name}: ${elapsed_time_ms} ms\033[0m"

            if [ $program_status -ne 0 ]; then
                echo -e "\033[1m\033[31mRuntime Error (exit code ${program_status})\033[0m"
                failed_count=$((failed_count + 1))
            elif [ -f "$answer_file" ]; then
                expected_normalized=$(mktemp)
                output_normalized=$(mktemp)
                diff_file=$(mktemp)
                normalize_for_compare "$answer_file" "$expected_normalized"
                normalize_for_compare "$output_file" "$output_normalized"

                if diff -u "$expected_normalized" "$output_normalized" > "$diff_file"; then
                    echo -e "\033[1m\033[32mAccepted\033[0m"
                    passed_count=$((passed_count + 1))
                else
                    echo -e "\033[1m\033[31mWrong Answer\033[0m"
                    echo -e "\033[33mDiff with expected output:\033[0m"
                    cat "$diff_file"
                    failed_count=$((failed_count + 1))
                fi

                rm -f "$expected_normalized" "$output_normalized" "$diff_file"
            else
                echo -e "\033[33mNo answer file found: ${answer_file}\033[0m"
                missing_answer_count=$((missing_answer_count + 1))
            fi

            rm -f "$output_file"
        else
            echo "No .txt files found in ./testcase/"
            exit 1
        fi
    done

    elapsed_time_ms=$(echo "scale=3; $all_case_time / 1000000" | bc)
    echo -e "\033[1m\033[34m\nExecution time for all cases: ${elapsed_time_ms} ms\033[0m"
    echo -e "\033[1m\033[32mPassed: ${passed_count}\033[0m"
    echo -e "\033[1m\033[31mFailed: ${failed_count}\033[0m"
    echo -e "\033[1m\033[33mMissing answer: ${missing_answer_count}\033[0m\n"
else
    echo "The directory ./testcase does not exist."
fi
