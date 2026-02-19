CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
SRCDIR = src

.PHONY: all clean test

all: super

super: $(SRCDIR)/main.cpp $(SRCDIR)/super.cpp $(SRCDIR)/super.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRCDIR)/main.cpp $(SRCDIR)/super.cpp

test_super: $(SRCDIR)/test_super.cpp $(SRCDIR)/super.cpp $(SRCDIR)/super.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRCDIR)/test_super.cpp $(SRCDIR)/super.cpp

test: test_super
	./test_super

clean:
	rm -f super test_super
