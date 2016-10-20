TARGET=$(which mockbot)
RELEASE=true sh build.sh && sudo cp bin/mockbot $TARGET && echo "Copied ./bin/mockbot to $TARGET"
