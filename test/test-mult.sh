echo "--- MOCKBOT COMPOSITE ---"
time bin/mockbot composite test/img/blankshirt.png test/img/tcb_spies14x16.png 409 192 665 760 multiply test/img/out1.png
echo "generated test/img/out1.png"
echo "--- IMAGEMAGICK COMPOSITE ---"
time convert -composite test/img/blankshirt.png test/img/tcb_spies14x16.png -compose multiply -alpha on -geometry 665x760+409+192 -depth 8 test/img/out.png
echo "generated test/img/out1.png"
