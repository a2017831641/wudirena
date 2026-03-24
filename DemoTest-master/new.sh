#!/bin/bash

destination_folder="./archive"
output_prefix="backup"

folders_to_compress=(
    "./cpp"
    "./py"
    "./testcase"
)

output_filename="archive"
timestamp=$(date +"%Y%m%d_%H%M%S")

if [ ! -d "$destination_folder" ]; then
    echo "The destination folder $destination_folder does not exist, creating it..."
    mkdir -p "$destination_folder"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create the destination folder!"
        exit 1
    fi
fi

output_path="$destination_folder/${output_filename}_${timestamp}.zip"

zip -r "$output_path" "${folders_to_compress[@]}"
if [ $? -eq 0 ]; then
    echo "Successfully compressed all folders into $output_path"
else
    echo "Failed to compress the folders"
    exit 1
fi

echo "Do you wish to renew? (n)"
read answer

# renew
if [ "$answer" != "${answer#[Nn]}" ] ;then
    echo "\033[1m\033[32m Exiting...\n\033[0m"
    exit 1
else
    echo "\033[1m\033[32m Renewing...\n\033[0m"
    
    rm -rf ./testcase/*
    touch ./testcase/0.txt

    rm -rf ./cpp/source/*
    touch ./cpp/source/main.cpp
    cat <<EOF > ./cpp/source/main.cpp
#include "helper.hpp"

using namespace std;

int main() {
    cout << "hello world" << endl;
    return 0;
}
EOF

    rm -rf ./py/*
    touch ./py/script.py

fi


