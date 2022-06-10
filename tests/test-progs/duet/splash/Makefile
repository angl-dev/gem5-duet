ifeq ($(SPLASH3_ROOT),)
$(error SPLASH-3 root directory not specified. Use `make SPLASH3_ROOT=xxx` or `export SPLASH3_ROOT=xxx` before calling make)
endif

ifeq ($(APP),)
$(error APP not set. Use e.g. `make APP=barnes`)
endif

SRCDIR := $(SPLASH3_ROOT)/codes/apps/$(APP)
SRCS := $(patsubst %.c.in,%.c,$(wildcard $(SRCDIR)/*.c.in))
INCS := $(patsubst %.h.in,%.h,$(wildcard $(SRCDIR)/*.h.in))

RVCC := riscv64-unknown-linux-gnu-gcc -DRISCV
RVFLAGS := -static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
CFLAGS := -O3 -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200112 -std=c11 -g -fno-strict-aliasing -I$(SRCDIR)
BIN := ../bin/riscv/linux/$(APP)

ifeq ($(APP),barnes)
CFLAGS += -DWITH_NO_OPTIONAL_LOCKS -DVERBOSE -DQUADPOLE
endif

all: $(BIN) $(BIN).duet

$(SRCS): %.c : %.c.in
	$(MAKE) -C $(SRCDIR) $(notdir $@)

$(INCS): %.h : %.h.in
	$(MAKE) -C $(SRCDIR) $(notdir $@)

$(BIN): $(SRCS) $(INCS)
	$(RVCC) -o $@ $(SRCS) $(RVFLAGS) $(CFLAGS) -lm

$(BIN).duet: $(SRCS) $(INCS)
	$(RVCC) -o $@ $(SRCS) $(RVFLAGS) $(CFLAGS) -DDUET -lm
