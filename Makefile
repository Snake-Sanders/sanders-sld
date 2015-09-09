UNAME_S := $(shell uname -s)
OBJDIR := obj

ifeq ($(UNAME_S),Linux)
	CCFLAGS += -D LINUX
	CXX = g++
endif
ifeq ($(UNAME_S),Darwin)
	CCFLAGS += -D OSX
	CXX = clang++
endif

SDL = -lSDL2
# If your compiler is a bit older you may need to change -std=c++11 to -std=c++0x
CXX_FLAGS = -Wall -c -std=c++11
LD_FLAGS = $(SDL) -lmosquitto
EXE = main

CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

all: $(EXE)

$(EXE): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LD_FLAGS)

obj/%.o: src/%.cpp | $(OBJDIR)
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm obj/*.o && rm $(EXE)

