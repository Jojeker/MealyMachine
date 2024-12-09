#!/bin/bash

PASS_PATH="../LLVMGlobalInstrumentation"
PASS_FILE="GlobalAccessInstrumentation.so"
PASS_NAME="GlobalAccessInstrumentation"

# Collect all arguments
ARGS=("$@")

# Initialize variables
OUTPUT_FILE=""
COMPILE_ONLY="no"
prev_arg=""

# Process arguments to find output file and check if compiling only
for arg in "$@"; do
    # Output file must be chagned
    if [[ "$prev_arg" == "-o" ]]; then
        OUTPUT_FILE="$arg"
    fi
    if [[ "$arg" == "-c" ]]; then
        COMPILE_ONLY="yes"
    fi
    prev_arg="$arg"
done

# Set default output file if not specified
if [[ -z "$OUTPUT_FILE" ]]; then
    if [[ "$COMPILE_ONLY" == "yes" ]]; then
        OUTPUT_FILE="a.out"
    else
        OUTPUT_FILE="a.out"
    fi
fi

# Create temporary filenames
TMP_BC_FILE="$(mktemp /tmp/wrapper_temp_XXXXXX.bc)"
TMP_OBJ_FILE="$(mktemp /tmp/wrapper_temp_XXXXXX.o)"

# Function to clean up temporary files
cleanup() {
    rm -f "$TMP_BC_FILE" "$TMP_OBJ_FILE"
}
trap cleanup EXIT

# Now the actual compilation process (abstracted from make)

# **Compile source files to LLVM bitcode**
clang++ -emit-llvm -c "${ARGS[@]}" -o "$TMP_BC_FILE"
# **Instrument the LLVM bitcode**
opt -load-pass-plugin="$PASS_PATH/$PASS_FILE" --passes="$PASS_NAME" "$TMP_BC_FILE" -o "$TMP_BC_FILE"
# **Compile the instrumented bitcode to an object file**
clang++ -c "$TMP_BC_FILE" -o "$TMP_OBJ_FILE"

if [[ "$COMPILE_ONLY" == "yes" ]]; then
    # **Output the object file**
    mv "$TMP_OBJ_FILE" "$OUTPUT_FILE"
else
    # **Link the object file to create the executable**
    clang++ "$TMP_OBJ_FILE" -o "$OUTPUT_FILE"
fi