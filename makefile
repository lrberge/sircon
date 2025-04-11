
CPPFLAGS:=-Wall -Wextra -pedantic -g -O0
LARGE_STACK:=-Wl,--stack,10000000
LINKER_FLAGS:=$(CPPFLAGS)

.PHONY: all bcon clean
all: sircon


sircon:
	cd src
	make
	cd ..


# Other targets

test_index: tests/test_to_index.exe
tests/test_to_index.o: tests/test_to_index.cpp to_index.cpp to_index.hpp
tests/test_to_index.exe: tests/test_to_index.o to_index.o
	g++ $(LINKER_FLAGS) $^ -o $@

test_shellrun: tests/test_shellrun.exe
tests/test_shellrun.exe: shellrun.o
	g++ $(LINKER_FLAGS) $^ tests/test_shellrun.cpp -o $@
	
test_stringtools: tests/test_stringtools.exe
tests/test_stringtools.exe: stringtools.o
	g++ $(LINKER_FLAGS) $^ tests/test_stringtools.cpp -o $@


clean:
	rm -f *.o


