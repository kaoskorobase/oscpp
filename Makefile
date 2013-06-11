all: test README

.PHONY: all test README clean
test:
	@echo "Building and executing tests"
	$(MAKE) -C test test
README:
	@echo "Building and executing README"
	$(MAKE) -C test README
	./test/README
clean:
	$(MAKE) -C test clean
