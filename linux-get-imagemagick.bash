#!/bin/bash

apt-get purge graphicsmagick-libmagick-dev-compat
apt-get install libmagick++-dev libwebp-dev
ln -s /usr/lib/x86_64-linux-gnu/$(ls /usr/lib/x86_64-linux-gnu | grep ImageMagick | head -n1)/bin-Q16/Magick++-config /usr/bin/
