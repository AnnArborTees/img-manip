wget http://www.imagemagick.org/download/ImageMagick.tar.gz -O ../ImageMagick.tar.gz
$(cd .. && tar xvzf ImageMagick.tar.gz )
$(cd ../ImageMagick* && ./configure --with-quantum-depth=8)
$(cd ../ImageMagick* && make)
$(cd ../ImageMagick* && sudo make install)
echo DONE
