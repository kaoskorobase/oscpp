DEBUG := 1
OPT := 1

HEADERS := $(wildcard ../include/oscpp/*.hpp) $(wildcard ../include/oscpp/detail/*.hpp)
AUTOCHECK_HEADERS := $(wildcard autocheck/include/autocheck/*.hpp)

ifeq ($(DEBUG),1)
	CXXFLAGS_DEBUG := -ggdb
endif

ifeq ($(OPT),1)
	CXXFLAGS_OPT := -O2
else
	CXXFLAGS_OPT := -O0
endif

CXXFLAGS = \
	-std=c++11 -stdlib=libc++ \
	-Wall -Wextra -Wno-unused-parameter \
	-I../include -Iautocheck/include \
	$(CXXFLAGS_DEBUG) \
	$(CXXFLAGS_OPT)

all: oscpp_autocheck

oscpp_autocheck: $(HEADERS) $(AUTOCHECK_HEADERS) oscpp_autocheck.cpp
	$(CXX) -o $@ $(CXXFLAGS) oscpp_autocheck.cpp

.PHONY: test
test: oscpp_autocheck
	./oscpp_autocheck

README.cpp: ../README.md
	runhaskell ../tools/mdcode.hs < $< > $@

README: $(HEADERS) README.cpp
	$(CXX) -o $@ $(CXXFLAGS) README.cpp

.PHONY: clean
clean:
	rm -f oscpp_autocheck
	rm -f README.cpp README
	rm -rf *.dSYM
