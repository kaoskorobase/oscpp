HEADERS = ../oscpp/client.hpp \
	  ../oscpp/error.hpp \
	  ../oscpp/host.hpp \
	  ../oscpp/server.hpp \
	  ../oscpp/stream.hpp \
	  ../oscpp/types.hpp
SOURCES = oscpp_test.cpp
CXXFLAGS = -I../ -O2

oscpp_test: $(HEADERS) $(SOURCES)
	$(CXX) -o $@ $(CXXFLAGS) $(SOURCES)

clean:
	rm -f oscpp_test
