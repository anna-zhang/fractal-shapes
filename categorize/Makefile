LDFLAGS_COMMON = -framework Accelerate -framework GLUT -framework OpenGL -lstdc++ -L/opt/homebrew/lib/ -ljpeg -lpng
CFLAGS_COMMON = -c -Wall -I./ -I/opt/homebrew/include/ -O3

# calls:
CC         = g++
CFLAGS     = ${CFLAGS_COMMON}
LDFLAGS    = ${LDFLAGS_COMMON}
EXECUTABLE = categorize

SOURCES    = categorize_images.cpp
OBJECTS    = $(SOURCES:.cpp=.o)

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
