
CPPFLAGS:=-Wall -Wextra -pedantic -g -O0
LARGE_STACK:=-Wl,--stack,10000000
LINKER_FLAGS:=$(CPPFLAGS)

.PHONY: all clean
all: sircon


sircon:
	cd ./src &&	$(MAKE) -j8
	
release:
	cd ./src && $(MAKE) clean	&& $(MAKE) release=1 -j8

clean:
	cd ./src && $(MAKE) clean -j8

# Other targets

test_index: tests/test_to_index.exe
tests/test_to_index.o: tests/test_to_index.cpp src/to_index.cpp src/to_index.hpp
tests/test_to_index.exe: tests/test_to_index.o src/to_index.o
	g++ $(LINKER_FLAGS) $^ -o $@

test_shellrun: tests/test_shellrun.exe
tests/test_shellrun.exe: src/shellrun.o
	g++ $(LINKER_FLAGS) $^ tests/test_shellrun.cpp -o $@
	
test_stringtools: tests/test_stringtools.exe
tests/test_stringtools.exe: src/stringtools.o
	g++ $(LINKER_FLAGS) $^ tests/test_stringtools.cpp -o $@



