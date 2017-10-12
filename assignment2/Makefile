BASE = asst2

all: $(BASE)

OS := $(shell uname -s)

ifeq ($(OS), Linux)
  CPPFLAGS = -I/opt/glew/include
  LDFLAGS += -L/opt/glew/lib -L/usr/X11R6/lib
  LIBS += -lGL -lGLU -lglut -lGLEW
endif

ifeq ($(OS), Darwin) # MacOS
  CPPFLAGS += -D__MAC__ -stdlib=libc++
  LDFLAGS += -framework GLUT -framework OpenGL
endif

ifdef OPT
  #turn on optimization
  CXXFLAGS += -O2
else
  #turn on debugging
  CXXFLAGS += -g
endif

CXX = g++ -Wno-deprecated-declarations

OBJ = $(BASE).o ppm.o glsupport.o

$(BASE): $(OBJ)
	$(LINK.cpp) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ) $(BASE)
