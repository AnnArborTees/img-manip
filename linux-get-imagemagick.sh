wget http://www.imagemagick.org/download/ImageMagick.tar.gz -O ../ImageMagick.tar.gz
echo "Extracting archive..."
(cd .. && tar xvzf ImageMagick.tar.gz )
sleep 1

echo "Running ./configure..."
(cd ../ImageMagick-7.0.2-5 && ./configure --with-quantum-depth=8)

echo "Running make"
(cd ../ImageMagick-7.0.2-5 && make)

echo "Running make insatll"
(cd ../ImageMagick-7.0.2-5 && sudo make install)

echo DONE
