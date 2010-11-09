#.o file in build dir for each .cpp file (plus main.cpp)
OBJS    = $(patsubst %.cpp,build/%.o, $(wildcard */*.cpp) main.cpp)

#needed libraries
LIBLST	= sdl gl ode

#library flag and inclusion arguments
LIBS	= `pkg-config --libs $(LIBLST)`
FLAGS	= `pkg-config --cflags $(LIBLST)`

#gcc options:
#generate dependecy files, optimization level 2, all warnings,
#optimization for architecture and inclusion of library compiler flags
ARGS	= -MD -O2 -pipe -Wall -march=native -mtune=native $(FLAGS)

#good default rule name, "all"
all: ../rcx

#final normal target
../rcx: build $(OBJS)
	@echo "Linking..."
	@g++ -o ../rcx $(OBJS) $(LIBS) $(ARGS)

#prerequisites (.d files for obj files)
#(note: generated efter last compile)
-include $(OBJS:.o=.d)

#universal build rule
$(OBJS): build/%.o:%.cpp
	@echo "Compiling $(<)..."
	@g++ -c -o $@ $< $(ARGS)

#if build dir not exists, create it (plus subdirs)
build:
	@echo "Creating build directories..."
	@mkdir -p build/shared build/loaders build/interface build/simulation

clean:
	@echo "Removing build files..."
	@-rm -rf build

distclean: clean
	@echo "Removing rcx binary..."
	@-rm -f ../rcx


#static windows 32b compilation (reuses this makefiles with rewritten variables)
#static: contains all needed libraries (since these people lack package managers)
#stripped: no symbols (since they usually can't debug, just small file size instead)
w32:
	@echo "Compiling \"release\" (static+stripped) windows binary..."
	@make \
		LIBS='`sdl-config --static-libs --libs` `ode-config --libs` -lopengl32 -static -lstdc++ -lwinmm -lgdi32' \
		FLAGS='`sdl-config --cflags` `ode-config --cflags`' \
		ARGS='-MD -O2 -pipe -Wall -march=i686 -mtune=i686 $$(FLAGS) -Dwindows'
	@strip ../rcx.exe

#these rules don't generate any files
.PHONY: all w32 clean distclean

