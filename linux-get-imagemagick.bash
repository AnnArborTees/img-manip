#!/bin/bash

sudo apt-get purge graphicsmagick-libmagick-dev-compat
sudo apt-get install libmagick++-dev
sudo apt install libmagick++-dev
sudo ln -s /usr/lib/x86_64-linux-gnu/ImageMagick-6.8.9/bin-Q16/Magick++-config /usr/bin/
