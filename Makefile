# Compiler and flags
LLVM_CONFIG = llvm-config
CXX = $(shell $(LLVM_CONFIG) --cxx)
CXXFLAGS = -std=c++17 -Wall -Wextra -g -gdwarf
CXXPASSFLAGS = -fpass-plugin=$(PASS_PATH)/$(PASS_FILE)
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)

# Instrumentation pass (overwrite if needed)
PASS_PATH ?= ../LLVMGlobalInstrumentation
PASS_NAME = GlobalAccessInstrumentation
PASS_FILE = $(PASS_NAME).so


# LLVM tools
OPT = opt
CLANG = clang++

# Source and executable
SRC = mealy.cpp
BC = mealy.bc
OBJ = mealy.o
EXEC = mealy_machine
EXEC_FAST = mealy_machine_fast

# Targets
all: $(EXEC)

# Fast build with -Xclang for instrumentation
all-fast: $(EXEC_FAST)

# Compile and instrument the source directly to an executable
$(EXEC_FAST): $(SRC) build-pass
	$(CLANG) $(CXXFLAGS) $(CXXPASSFLAGS) $(SRC) -o $(EXEC_FAST)

build-pass:
	make -C $(PASS_PATH)

unmod-ir: $(SRC)
	$(CLANG) -emit-llvm -S $(SRC) -o mealy.ll

mod-ir: $(SRC)
	$(CLANG) -emit-llvm -S instrumented.bc -o mod-mealy.ll

# Generate LLVM IR
$(BC): $(SRC)
	$(CLANG) $(CXXFLAGS) -emit-llvm -c -S $(SRC) -o $(BC)

# Instrument the IR with the pass
instrument: $(BC)
	$(OPT) -load-pass-plugin=$(PASS_PATH)/$(PASS_FILE) --passes="$(PASS_NAME)" -o instrumented.bc $(BC)

# Compile the instrumented IR to an executable
$(EXEC): build-pass instrument
	$(CLANG) $(CXXFLAGS) instrumented.bc -o $(EXEC) $(LDFLAGS) 

clean:
	rm -f $(BC) instrumented.bc $(OBJ) $(EXEC) $(EXEC_FAST)

.PHONY: all clean instrument
