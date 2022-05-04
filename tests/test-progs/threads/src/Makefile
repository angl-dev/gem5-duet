x86: ../bin/x86/linux/threads

riscv: ../bin/riscv/linux/threads

../bin/x86/linux/threads: threads.cpp
	g++ -o ../bin/x86/linux/threads threads.cpp -pthread -std=c++11

../bin/riscv/linux/threads: threads.cpp
	riscv64-unknown-linux-gnu-g++ -o $@ $< -O1 -g -static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
