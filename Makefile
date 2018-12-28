.PHONY: build clean distclean test

build: build/CMakeCache.txt
	cd build && ninja

build/CMakeCache.txt:
	mkdir -p build
	cd build && cmake -G Ninja ..

clean:
	cd build && ninja clean

distclean:
	rm -rf build

test: build
	cd build && ctest -V
