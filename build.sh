if [ "$RELEASE" = "true" ]; then
  echo "Release build"
  FLAGS="-O3"
else
  echo "Debug build"
  FLAGS="-ggdb -O0"
fi

gcc -c -std=c99 -o lib/cJSON.o lib/cJSON.c
g++ src/*.cpp lib/cJSON.o -std=c++11 -Ilib -lpng -o bin/mockbot $(echo $FLAGS) `Magick++-config --cxxflags --cppflags --ldflags --libs` -UMAGICKCORE_QUANTUM_DEPTH -DMAGICKCORE_QUANTUM_DEPTH=8

if [ "$INSTALL" = "true" ]; then
  echo "Installing"
  cp bin/mockbot /usr/bin/mockbot
fi
