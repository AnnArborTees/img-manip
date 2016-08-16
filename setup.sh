mkdir -p lib
wget https://github.com/DaveGamble/cJSON/raw/master/cJSON.c -O lib/cJSON.c
wget https://github.com/DaveGamble/cJSON/raw/master/cJSON.h -O lib/cJSON.h
wget "http://downloads.sourceforge.net/project/utfcpp/utf8cpp_2x/Release%202.3.4/utf8_v2_3_4.zip?r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Futfcpp%2Ffiles%2Futf8cpp_2x%2FRelease%25202.3.4%2Futf8_v2_3_4.zip%2Fdownload%3Fuse_mirror%3Dpilotfiber&ts=1471293028&use_mirror=heanet" -O lib/utf8.zip
unzip lib/utf8.zip source/utf8/checked.h source/utf8/unchecked.h source/utf8/core.h -d lib
mkdir -p lib/utf8
mv lib/source/utf8/*.h lib/utf8
rm -rf lib/source
rm lib/utf8.zip
