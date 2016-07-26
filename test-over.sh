time bin/mockbot composite test/blankshirt-rgb.png test/tcb_spies14x16.png 409 192 665 760 over test/out1.png
time convert -composite test/blankshirt-rgb.png test/tcb_spies14x16.png -alpha on -geometry 665x760+409+192 -depth 8 test/out.png

echo "Generated test/out1.png from mockbot and test/out.png from imagemagick"
