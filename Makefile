CXXFLAGS := -std=gnu++14 -Wall -Wextra -pedantic -pthread -Wswitch-enum

PYTHON := python2
GETOS := $(PYTHON) $(NACL_SDK_ROOT)/tools/getos.py
OSHELPERS = $(PYTHON) $(NACL_SDK_ROOT)/tools/oshelpers.py
OSNAME := $(shell $(GETOS))
RM := $(OSHELPERS) rm

PNACL_TC_PATH := $(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_pnacl)
PNACL_CXX := $(PNACL_TC_PATH)/bin/pnacl-clang++
PNACL_FINALIZE := $(PNACL_TC_PATH)/bin/pnacl-finalize
CPPFLAGS := -I$(NACL_SDK_ROOT)/include -I$(NACL_SDK_ROOT)/boost/include
LDFLAGS := -L$(NACL_SDK_ROOT)/lib/pnacl/Release -lppapi_cpp -lppapi

.PHONY: all clean

all: test.pexe

clean:
	-$(RM) test.pexe test.bc

test.cc: window.hpp

window.hpp: jsvar.hpp

%.bc: %.cc
	$(PNACL_CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LDFLAGS) -o $@

%.pexe: %.bc
	$(PNACL_FINALIZE) $< -o $@

HTTPD_PY := $(PYTHON) $(NACL_SDK_ROOT)/tools/httpd.py

.PHONY: serve
serve: all
	$(HTTPD_PY) -C $(CURDIR) --no-dir-check
