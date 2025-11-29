ENGINE := KamekInclude
GAMESOURCE := GameSource
PULSAR := PulsarEngine
KAMEK := KamekLinker/Kamek.exe
CC := C:/Users/sauxy/Documents/CLT/mwcceppc.exe
RIIVO := C:/Users/sauxy/Documents/MKM-Testing/MKMayhem
RIIVO2 := C:/Users/sauxy/Documents/MKM-Pulsar-Creator/output/MKMayhem

CFLAGS := -I- -i $(ENGINE) -i $(GAMESOURCE) -i $(PULSAR) \
  -opt all -inline auto -enum int -proc gekko -fp hard -sdata 0 -sdata2 0 -maxerrors 1 -func_align 4
DEFINE :=
EXTERNALS := -externals=$(GAMESOURCE)/symbols.txt -externals=$(GAMESOURCE)/anticheat.txt -versions=$(GAMESOURCE)/versions.txt


CPPFILES := $(shell find $(PULSAR) -name '*.cpp')

OBJECTS := $(patsubst %.cpp,build/%.o,$(subst $(PULSAR)/,,$(CPPFILES)))

$(shell mkdir -p build)

all: build/Code.pul

build/kamek.o: $(ENGINE)/kamek.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

build/RuntimeWrite.o: $(ENGINE)/RuntimeWrite.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: $(PULSAR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEFINE) -c -o $@ $<

build/Code.pul: build/kamek.o build/RuntimeWrite.o $(OBJECTS)
	$(KAMEK) build/kamek.o build/RuntimeWrite.o $(OBJECTS) -dynamic $(EXTERNALS) -output-combined=build/Code.pul

clean:
	rm -rf build/*.o build/Code.pul

.PHONY: all clean force_link install

force_link: build/Code.pul

install: force_link
	@echo Copying binaries to $(RIIVO)/Binaries...
	@echo Copying binaries to $(RIIVO2)/Binaries...
	@mkdir -p $(RIIVO)/Binaries
	@mkdir -p $(RIIVO2)/Binaries
	@cp build/Code.pul $(RIIVO)/Binaries
	@cp build/Code.pul $(RIIVO2)/Binaries