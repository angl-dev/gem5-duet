SRCS := $(wildcard *.cpp)
BIN := $(patsubst %.cpp,%,${SRCS}) 
TARGET := $(addprefix ../bin/riscv/linux/,${BIN})
LPTHREAD = -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive

all: ${TARGET}

${TARGET}: ../bin/riscv/linux/% : %.cpp
	mkdir -p $(dir $@)
	riscv64-unknown-linux-gnu-g++ -o $@ $< -O0 -g -static $(LPTHREAD)
