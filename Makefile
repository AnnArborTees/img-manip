CXX=g++
FLAGS=lib/cJSON.o -std=c++11 -Ilib -lpng $(shell Magick++-config --cppflags --ldflags --libs)
OPTIMIZE=-O2 -ffast-math -flto
DEBUG=-ggdb -O0

all: bin/mockbot
mockbot: bin/mockbot

debug: lib lib/cJSON.o $(wildcard src/*.cpp) $(wildcard src/*.h)
	mkdir -p bin
	$(CXX) $(FLAGS) $(DEBUG) $(wildcard src/*.cpp) -o bin/mockbot

bin/mockbot: lib lib/cJSON.o $(wildcard src/*.cpp) $(wildcard src/*.h)
	mkdir -p bin
	$(CXX) $(FLAGS) $(OPTIMIZE) $(wildcard src/*.cpp) -o $@

update: bin/mockbot
	cp bin/mockbot /usr/bin/mockbot

install: bin/mockbot
	cp bin/mockbot /usr/bin/mockbot

lib/cJSON.o: lib lib/cJSON.c lib/cJSON.h
	gcc $(OPTIMIZE) -c -std=c99 -o $@ lib/cJSON.c

lib:
	sh setup.sh

clean:
	rm bin/*
	rm lib/cJSON.o
