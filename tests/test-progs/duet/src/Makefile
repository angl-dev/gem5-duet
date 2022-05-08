riscv: ../bin/riscv/linux/test_driver

../bin/riscv/linux/test_driver: test_driver.cpp
	mkdir -p $(dir $@)
	riscv64-unknown-linux-gnu-g++ -o $@ $< -O0 -g -static
