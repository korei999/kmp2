MAKEFLAGS := --jobs=$(shell nproc) --output-sync=target 

# CXX := c++ -fdiagnostics-color=always
CXX := clang++ -fcolor-diagnostics -fansi-escape-codes -fdiagnostics-format=msvc
# compile wayland glue code with c compiler due to linkage issues

WARNINGS := -Wall -Wextra -fms-extensions -Wno-missing-field-initializers

include debug.mk

PKGS := libpipewire-0.3 ncurses
PKG := $(shell pkg-config --cflags $(PKGS))
PKG_LIB := $(shell pkg-config --libs $(PKGS))

CXXFLAGS := -std=c++23 $(PKG)
LDFLAGS := $(PKG_LIB)

XDG_SHELL := $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
POINTER_CONSTRAINTS := $(WAYLAND_PROTOCOLS_DIR)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml
RELATIVE_POINTER := $(WAYLAND_PROTOCOLS_DIR)/unstable/relative-pointer/relative-pointer-unstable-v1.xml

SRCD := ./src
BD := ./build
BIN := kmp
EXEC := $(BD)/$(BIN)

SRCS := $(shell find $(SRCD) -name '*.cc')
OBJS := $(SRCS:%=$(BD)/%.o)
DEPS := $(OBJS:.o=.d)
INC_DIRS := $(shell find $(SRCS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS := $(INC_FLAGS) -MMD -MP

all: CXX += -flto=auto $(SAFE_STACK) -DFPS_COUNTER
all: CXXFLAGS += -g -O3 -march=native -ffast-math $(WARNINGS) -DNDEBUG
all: $(EXEC)

debug: CXX += $(ASAN)
debug: CXXFLAGS += -g -O0 $(DEBUG) $(WARNINGS) $(WNO)
debug: LDFLAGS += -fuse-ld=mold
debug: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BD)/%.cc.o: %.cc Makefile debug.mk
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean tags
clean:
	rm -rf $(BD)

tags:
	ctags -R --language-force=C++ --extras=+q+r --c++-kinds=+p+l+x+L+A+N+U+Z

-include $(DEPS)
