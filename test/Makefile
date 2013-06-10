DEBUG := 1
OPT := 1

HEADERS := $(wildcard ../oscpp/*.hpp) $(wildcard ../oscpp/detail/*.hpp)
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
	-I.. -Iautocheck/include \
	$(CXXFLAGS_DEBUG) \
	$(CXXFLAGS_OPT)

all: oscpp_autocheck

oscpp_autocheck: $(HEADERS) oscpp_autocheck.cpp
	$(CXX) -o $@ $(CXXFLAGS) oscpp_autocheck.cpp

clean:
	rm -rf oscpp_autocheck oscpp_autocheck.dSYM
