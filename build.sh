if [ "$RELEASE" = "true" ]; then
  FLAGS="-O2"
  echo "Building release binary"
else
  echo "Building debug binary"
  FLAGS="-ggdb -O0"
fi

gcc src/*.c -lpng -o bin/amazon-resize $(echo $FLAGS)
