


ifdef release
CPPFLAGS:=-Wall -Wextra -pedantic -O2
BINPATH:=../bin/
else 
CPPFLAGS:=-Wall -Wextra -pedantic -g -O0
BINPATH:=
endif

LARGE_STACK:=-Wl,--stack,10000000
LINKER_FLAGS:=$(CPPFLAGS)

.PHONY: all bcon clean
all: sircon.exe

util.o: util.cpp util.hpp

stringtools.o: stringtools.cpp stringtools.hpp constants.hpp metastringvec.hpp util.hpp

pathmanip.o: pathmanip.cpp pathmanip.hpp stringtools.hpp constants.hpp

clipboard.o: clipboard.cpp clipboard.hpp stringtools.hpp

to_index.o: to_index.cpp to_index.hpp

R.o: R.hpp R.cpp

console.o: console.cpp console.hpp constants.hpp VTS.hpp stringtools.hpp clipboard.hpp pathmanip.hpp metastringvec.hpp autocomplete.hpp program_options.hpp shellrun.hpp specialfunctions.hpp history.hpp console_util.hpp

history.o: history.hpp history.cpp stringtools.hpp util.hpp console.hpp console_util.hpp

rlanguageserver.o: rlanguageserver.cpp rlanguageserver.hpp console.hpp constants.hpp VTS.hpp stringtools.hpp R.hpp R.cpp cache.hpp RAutocomplete.hpp program_options.hpp
rlanguageserver.o: CPPFLAGS+=-Wno-cast-function-type -Wno-unused-parameter

cache.o: cache.cpp cache.hpp

program_options.o: program_options.hpp program_options.cpp VTS.hpp stringtools.hpp

shortcuts.o: shortcuts.hpp shortcuts.cpp stringtools.hpp metastringvec.hpp

shellrun.o: shellrun.hpp shellrun.cpp util.hpp

specialfunctions.o: specialfunctions.cpp specialfunctions.hpp console.hpp stringtools.hpp util.hpp program_options.hpp

autocomplete.o: autocomplete.cpp autocomplete.hpp stringtools.hpp metastringvec.hpp util.hpp console.hpp

RAutocomplete.o: RAutocomplete.cpp RAutocomplete.hpp autocomplete.cpp autocomplete.hpp stringtools.hpp R.hpp metastringvec.hpp to_index.hpp util.hpp

sircon.o: rlanguageserver.o sircon.cpp console.hpp constants.hpp VTS.hpp stringtools.hpp clipboard.hpp pathmanip.hpp

%.o: %.cpp
	g++ $(CPPFLAGS) -c $< -o $@

sircon.exe: sircon.o console.o stringtools.o clipboard.o pathmanip.o to_index.o rlanguageserver.o R.o cache.o autocomplete.o RAutocomplete.o util.o program_options.o shellrun.o specialfunctions.o history.o shortcuts.o
	g++ $(LINKER_FLAGS) $(LARGE_STACK) $^ -o $(BINPATH)$@

clean:
	rm -f *.o


