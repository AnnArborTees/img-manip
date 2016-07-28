if [ "$RELEASE" = "true" ]; then
  echo "Release build"
  FLAGS="-O2 -ffast-math"
else
  echo "Debug build"
  FLAGS="-ggdb -O0"
fi

gcc -c -std=c99 -o lib/cJSON.o lib/cJSON.c
g++ src/*.cpp lib/cJSON.o -std=c++11 -Ilib -lpng -o bin/mockbot $(echo $FLAGS) `Magick++-config --cppflags --ldflags --libs`

if [ "$INSTALL" = "true" ]; then
  echo "Installing"
  cp bin/mockbot /usr/bin/mockbot
fi
