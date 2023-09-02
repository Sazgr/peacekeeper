EXE := peacekeeper-dev

SOURCES := src/*.cpp

CXX := g++

CXXFLAGS := -pthread -std=c++17 -Ofast -DNDEBUG -m64 -mpopcnt -msse2 -mavx2 -static -DVERSION=-1

LINKER :=

SUFFIX :=

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
else
	SUFFIX :=
	LINKER := -lm
endif

OUT := $(EXE)$(SUFFIX)


$(EXE): $(SOURCES)
	$(CXX) $^ $(CXXFLAGS) -o $(OUT) $(LINKER) 
