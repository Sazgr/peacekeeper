EXE = peacekeeper-dev
EVALFILE = src/default.nn

SOURCES := src/*.cpp

CXX := g++

CXXFLAGS := -pthread -std=c++17 -O3 -ffast-math -DNDEBUG -march=native -static -DVERSION=-1 -DNETWORK_FILE=\"$(EVALFILE)\"

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
