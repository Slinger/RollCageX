#.o file in build dir for each .cpp file (plus main.cpp)
OBJS    = $(patsubst %.cpp,build/%.o, $(wildcard */*.cpp) main.cpp)

#needed libraries
LIBLST	= sdl gl glu ode

#library flag and inclusion arguments
LIBS	= `pkg-config --libs $(LIBLST)` -lm
FLAGS	= `pkg-config --cflags $(LIBLST)`

#gcc options:
#generate dependecy files, optimization level 2, all warnings,
#optimization for architecture and inclusion of library compiler flags
ARGS	= -MD -O2 -pipe -Wall -march=native -mtune=native $(FLAGS)

#good default rule name, "all"
all: ../rcx

#final normal target
../rcx: builddirs $(OBJS)
	@echo "Linking..."
	@g++ -o ../rcx $(OBJS) $(LIBS) $(ARGS)

#prerequisites (.d files for obj files)
#(note: generated efter last compile)
-include $(OBJS:.o=.d)

#universal build rule
$(OBJS): build/%.o:%.cpp
	@echo "Compiling $(<)..."
	@g++ -c -o $@ $< $(ARGS)

#these rules don't generate any files
.PHONY: builddirs clean distclean

builddirs:
	@echo "Making sure build directories exists..."
	@mkdir -p build/shared build/events build/loaders build/graphics build/physics

clean:
	@echo "Removing build files"
	@-rm -rf build

distclean: clean
	@echo "Removing rcx binary"
	@-rm -f ../rcx

