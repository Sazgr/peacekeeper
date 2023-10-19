EXE := peacekeeper-dev

SOURCES := src/*.cpp

CXX := g++

CXXFLAGS := -pthread -std=c++17 -Ofast -DNDEBUG -march=native -static -DVERSION=-1 -Wl,--stack,8388608

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
