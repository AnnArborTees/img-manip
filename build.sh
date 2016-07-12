if [ "$RELEASE" = "true" ]; then
  echo "Release build"
  FLAGS="-O3"
else
  echo "Debug build"
  FLAGS="-ggdb -O0"
fi

g++ src/*.cpp -lpng -o bin/mockbot $(echo $FLAGS)

if [ "$INSTALL" = "true" ]; then
  echo "Installing"
  cp bin/mockbot /usr/bin/mockbot
fi
