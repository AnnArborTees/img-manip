#!/bin/sh

bin/mockbot "thumbnail" "--" "--" "test/vg_player210x12.png" "test/3.png" "500" "10" "over" "0" "1" "1500" "1762" "test/test-bug-out.png" > /dev/null

echo "test/test-bug-out.png"
