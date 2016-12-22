time bin/mockbot composite test/img/blankshirt-rgb.png test/img/tcb_spies14x16.png 409 192 665 760 over test/img/out1.png
time convert -composite test/img/blankshirt-rgb.png test/img/tcb_spies14x16.png -alpha on -geometry 665x760+409+192 -depth 8 test/img/out.png

echo "test/img/out1.png from mockbot and test/img/out.png from imagemagick"
