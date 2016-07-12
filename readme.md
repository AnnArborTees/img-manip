# MockBot image manipulation tool

## TO BUILD:

### In development

Just run `sh build.sh`

### In production

Run `RELEASE=true INSTALL=true sh build.sh`
You'll need root priveledges for that one. The INSTALL=true will make it copy the binary into /usr/bin
so you or your applications can call `mockbot` from anywhere.
