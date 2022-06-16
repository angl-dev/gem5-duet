ifeq ($(SPLASH3_ROOT),)
$(error SPLASH-3 root directory not specified. Use `make SPLASH3_ROOT=xxx` or `export SPLASH3_ROOT=xxx` before calling make)
endif

ifeq ($(APP),)
$(error APP not set. Use e.g. `make APP=barnes`)
endif

SRCDIR := $(SPLASH3_ROOT)/codes/apps/$(APP)
SRCS := $(patsubst %.c.in,%.c,$(wildcard $(SRCDIR)/*.c.in))
INCS := $(patsubst %.h.in,%.h,$(wildcard $(SRCDIR)/*.h.in))

RVCC := riscv64-unknown-linux-gnu-gcc
RVFLAGS := -static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
CFLAGS := -O3 -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200112 -DRISCV -std=c11 -g -fno-strict-aliasing -I$(SRCDIR)
LDFLAGS := -lm
BIN := ../bin/riscv/linux/$(APP)
ADDITIONAL_DEPS :=

all: $(BIN) $(BIN).duet

ifeq ($(APP),barnes)
CFLAGS += -DWITH_NO_OPTIONAL_LOCKS -DVERBOSE -DQUADPOLE
endif

ifeq ($(APP),fmm)
CFLAGS += -DPROFILE
endif

ifeq ($(APP),volrend)
CFLAGS += -I $(SRCDIR)/libtiff
LDFLAGS += -L $(SRCDIR)/libtiff -lrvtiff

ADDITIONAL_DEPS += $(SRCDIR)/libtiff/librvtiff.a

$(ADDITIONAL_DEPS): $(wildcard $(SRCDIR)/libtiff/*.c) $(wildcard $(SRCDIR)/libtiff/*.h)
	rm -f $(SRCDIR)/libtiff/*.a
	$(MAKE) -C $(SRCDIR)/libtiff g3states.h
	$(MAKE) -C $(SRCDIR)/libtiff CC=$(RVCC) AR=riscv64-unknown-linux-gnu-ar
	mv $(SRCDIR)/libtiff/libtiff.a $@

endif

$(SRCS): %.c : %.c.in
	$(MAKE) -C $(SRCDIR) $(notdir $@)

$(INCS): %.h : %.h.in
	$(MAKE) -C $(SRCDIR) $(notdir $@)

$(BIN): $(SRCS) $(INCS) $(ADDITIONAL_DEPS)
	$(RVCC) -o $@ $(SRCS) $(RVFLAGS) $(CFLAGS) $(LDFLAGS)

$(BIN).duet: $(SRCS) $(INCS) $(ADDITIONAL_DEPS)
	$(RVCC) -o $@ -DDUET $(SRCS) $(RVFLAGS) $(CFLAGS) $(LDFLAGS)
