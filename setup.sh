mkdir -p lib
wget https://github.com/DaveGamble/cJSON/raw/master/cJSON.c -O lib/cJSON.c
wget https://github.com/DaveGamble/cJSON/raw/master/cJSON.h -O lib/cJSON.h

if [ "$(which apt-get)" != "" ]; then
  echo "Need image magick"
fi
