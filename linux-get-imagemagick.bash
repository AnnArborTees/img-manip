#!/bin/bash

wget http://www.imagemagick.org/download/ImageMagick.tar.gz -O ImageMagick.tar.gz
echo "Extracting archive..."
tar xvzf ImageMagick.tar.gz
sleep 1

pushd ImageMagick-*

echo "Running ./configure..."
./configure --with-quantum-depth=8

echo "Running make"
make

echo "Running make insatll"
sudo make install

echo DONE
popd
