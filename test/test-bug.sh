#!/bin/sh

bin/mockbot "thumbnail" "--" "--" "test/img/vg_player210x12.png" "test/img/3.png" "500" "10" "over" "0" "1" "1500" "1762" "test/img/test-bug-out.png" > /dev/null

echo "test/img/test-bug-out.png"
