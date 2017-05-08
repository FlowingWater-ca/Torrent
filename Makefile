# default rule and rule shortcuts
all: torrent 
c: clean

CC  = g++
# directory structure
IDIR = ./include
ODIR = ./obj
SDIR = ./src

# objects, src and user libs
SOURCES = $(wildcard $(SDIR)/*.cpp)
INCLUDE = $(wildcard $(IDIR)/*.h)
OBJ := $(SOURCES:$(SDIR)/%.cpp=$(ODIR)/%.o)

# compile flags and libraries
FLAGS = -Wall -I$(IDIR)
LIBS = -L/usr/include/boost/ 

# executable
PRG = test

# create executable
torrent: $(OBJ)
	$(CC) $(OBJ) -o $(PRG) $(LIBS) $(FLAGS)

# compile src files into objects
$(ODIR)/%.o: $(SDIR)/%.cpp $(INCLUDE)
	$(CC) -c -o $@ $< $(FLAGS)

clean:
	rm -f $(PRG) $(ODIR)/*.o

