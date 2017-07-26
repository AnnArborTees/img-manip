#!/bin/bash

sudo apt-get purge graphicsmagick-libmagick-dev-compat
sudo apt-get install libmagick++-dev
sudo ln -s /usr/lib/x86_64-linux-gnu/ImageMagick-6.9.7/bin-Q16/Magick++-config /usr/bin/
