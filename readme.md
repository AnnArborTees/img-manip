# MockBot image manipulation tool

This repository contains the script we used to resize all our 14x16 150dpi images to 15x18 300dpi.
The files for that thing are `src/amazon-resize.c` and `convert-ready.rb` (if you wanna build the
executable you'll just have to use gcc manually).

The rest of this readme will refer to the mockup tool used by MockBot.

## REQUIREMENTS

### macOS

For mac, you'll likely want to run `brew install imagemagick --with-quantum-depth-8`

### Linux

``` bash
bash linux-get-imagemagick.bash
```

## TO BUILD:

### In development

``` bash
make debug
```

### In production

``` bash
make
make install # this creates a symlink so you only need to run it once
```

## TO USE:

Run `bin/mockbot help` to see the list of subcommands and how they're used.
Any number of subcommands can be specified in a single invocation of `mockbot`.

### Deployment

#### New method

The file `deploy.yml` is an [Ansible](https://www.ansible.com/how-ansible-works) playbook
that handles the installation of imagemagick, dependancies, and img-manip itself.

The deploy playbook assumes a `mockbot` group exists.

``` ini
# /etc/ansible/hosts
[mockbot]
10.0.0.242
10.0.1.111
```

``` bash
# To deploy 'mockbot'
ansible-playbook deploy.yml -f 10
```

#### Old method

From your local machine, run `ruby deploy.rb`. This uses the AWS CLI to find EC2 instances
with the "mockbot" role (same "Role" tag that `cap-ec2` uses.)

Some caveats:

* You must input your GitHub credentials. At some point, we should use git@github.com instead
  of https://github.com for this repository on the production servers.
* It is assumed that the `img-manip` repository is cloned into `/home/ubuntu/img-manip`
  on all production servers.

#### Manually

Just get it so that the mockbot executable is in PATH. Just running `make; sudo make install` on the production server should work just fine.

#### Helpful Introduction to libpng
- [Introduction from libpng.org](http://www.libpng.org/pub/png/libpng-manual.txt)
- [Manual](http://www.libpng.org/pub/png/libpng-1.4.0-manual.pdf)
- [Helpful OReilly Intro](ftp://ftp.oreilly.com/examples/gff/CDROM/SOFTWARE/SOURCE/LIBPNG/LIBPNG.TXT)
- [Intro from W3.org](https://www.w3.org/TR/PNG-Chunks.html)

