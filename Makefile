MAKEFLAGS := --jobs=$(shell nproc) --output-sync=target 

CXX := clang++ -fcolor-diagnostics -fansi-escape-codes -fdiagnostics-format=msvc

WARNINGS := -Wall -Wextra -fms-extensions -Wno-missing-field-initializers

include debug.mk

PKGS := libpipewire-0.3 ncursesw sndfile icu-io
PKG := $(shell pkg-config --cflags $(PKGS))
PKG_LIB := $(shell pkg-config --libs $(PKGS))

CXXFLAGS := -std=c++23 $(PKG)
LDFLAGS := $(PKG_LIB)

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

all: CXX := g++ -fdiagnostics-color=always -flto=auto
all: CXXFLAGS += -g -O3 -march=native -ffast-math $(WARNINGS) -DNDEBUG
all: $(EXEC)

debug: CXX += $(ASAN) $(SAFE_STACK)
debug: CXXFLAGS += -g3 -O0 $(DEBUG) $(WARNINGS) $(WNO)
debug: LDFLAGS += -fuse-ld=mold
debug: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BD)/%.cc.o: %.cc Makefile debug.mk
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

PREFIX = /usr/local

install: $(EXEC)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(EXEC) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(BIN)
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)

.PHONY: clean tags
clean:
	rm -rf $(BD)

tags:
	ctags -R --language-force=C++ --extras=+q+r --c++-kinds=+p+l+x+L+A+N+U+Z

-include $(DEPS)
