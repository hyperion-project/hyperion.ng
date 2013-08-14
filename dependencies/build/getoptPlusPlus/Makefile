SOURCES=getoptpp.cc test.cc
HEADERS=getoptpp.h
OBJECTS=$(SOURCES:.cc=.o)
LDFLAGS=
CXXFLAGS=-O0 -ggdb -Wall
CFLAGS=$(CXXFLAGS)
CC=g++
TARGET=getopt-test

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJECTS)
$(SOURCES): $(HEADERS)	

clean:
	rm -rf $(TARGET) $(OBJECTS) *~
