OS := $(shell uname -s)
HEADERS = $(wildcard ../oscpp/*.hpp) $(wildcard ../oscpp/detail/*.hpp)
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -I.. -Iautocheck/include -O2 -g -std=c++11 -stdlib=libc++

all: oscpp_autocheck

oscpp_test: $(HEADERS) oscpp_test.cpp
	$(CXX) -o $@ $(CXXFLAGS) oscpp_test.cpp

oscpp_autocheck: $(HEADERS) oscpp_autocheck.cpp
	$(CXX) -o $@ $(CXXFLAGS) oscpp_autocheck.cpp

clean:
	rm -f oscpp_autocheck oscpp_test
