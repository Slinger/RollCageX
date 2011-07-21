#
# ReCaged - a Free Software, Futuristic, Racing Simulator
#
# Copyright (C) 2009, 2010, 2011 Mats Wahlberg ("Slinger" on gorcx.net
# forum)
#
# This file is part of ReCaged.
#
# ReCaged is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ReCaged is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
#

#.o file in build dir for each .cpp file (plus main.cpp)
OBJS    = $(patsubst %.cpp,build/%.o, $(wildcard */*.cpp) main.cpp)

#needed libraries
LIBLST	= gl glew sdl ode

#library flag and inclusion arguments
LIBS	= `pkg-config --libs $(LIBLST)`
FLAGS	= `pkg-config --cflags $(LIBLST)`

#gcc options:
#generate dependecy files, optimization level 2, all warnings,
#optimization for architecture and inclusion of library compiler flags
ARGS	= -MD -O2 -pipe -Wall -march=native -mtune=native $(FLAGS)

#good default rule name, "all"
all: ../recaged

#final normal target
../recaged: build $(OBJS)
	@echo "Linking..."
	@g++ -o ../recaged $(OBJS) $(LIBS) $(ARGS)

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
	@echo "Removing recaged binary..."
	@-rm -f ../recaged


#static windows 32b compilation (reuses this makefiles with rewritten variables)
#static: contains all needed libraries (since these people lack package managers)
#stripped: no symbols (since they usually can't debug, just small file size instead)
w32:
	@echo "Compiling \"release\" (static+stripped) windows binary..."
	@make \
		LIBS='`sdl-config --static-libs --libs` `ode-config --libs` -lopengl32 -static -lstdc++ -lwinmm -lgdi32' \
		FLAGS='`sdl-config --cflags` `ode-config --cflags`' \
		ARGS='-MD -O2 -pipe -Wall -march=i686 -mtune=i686 $$(FLAGS) -Dwindows'
	@strip ../recaged.exe

#these rules don't generate any files
.PHONY: all w32 clean distclean

