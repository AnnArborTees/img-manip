# MockBot image manipulation tool

## REQUIREMENTS

### macOS

For mac, you'll likely want to run `brew install imagemagick --with-quantum-depth-8`

### Linux

Get your hands on imagemagick if you don't have it already.

## TO BUILD:

First,

```
git clone https://github.com/AnnArborTees/img-manip
cd img-manip
sh setup.sh
```

The `setup.sh` script will download dependencies into the `lib` folder. These dependencies are
* https://github.com/kbranigan/cJSON for parsing json character set offsets,
* and http://utfcpp.sourceforge.net/ for utf8 support.

### In development

Run `sh build.sh` or `RELEASE=true sh build.sh`

### In production

If you already have `mockbot` in your PATH, run `sh update.sh`.

Otherwise, do `RELEASE=true sh build.sh` then add `bin/mockbot` to your PATH somehow.
(copy it to `/usr/bin`, add `./bin` to PATH, whatever.)
