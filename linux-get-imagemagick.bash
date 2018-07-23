#!/bin/bash

sudo apt-get purge graphicsmagick-libmagick-dev-compat
sudo apt-get install libmagick++-dev libwebp-dev
sudo ln -s /usr/lib/x86_64-linux-gnu/$(ls /usr/lib/x86_64-linux-gnu | grep ImageMagick | head -n1)/bin-Q16/Magick++-config /usr/bin/
