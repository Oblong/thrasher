A texture memory thrasher.

For now, this project is using the [tup](http://gittup.org/tup/) build system.

Dependencies (Ubuntu)
-
```
sudo apt-get install libglfw3-dev
sudo apt-add-repository 'deb http://ppa.launchpad.net/anatol/tup/ubuntu precise main'
sudo apt-get update
sudo apt-get install tup
```

If you just need to build once, and you don't wish to install tup, you can just
run the g++ command located in the file named Tupfile.

Build
-
```
git submodule update --init
tup init
tup upd
```

Usage
-

```
  ./thrash {OPTIONS}

    A texture memory thrasher

  OPTIONS:

      --help                            Display this message
      -t[TEXELS],
      --texture-size=[TEXELS]           The maximum texture dimension in texels
                                        (largest texture is TEXELSxTEXELS). Note
                                        that if this value is more than the
                                        driver supports, the driver's maximum
                                        value will be used.
      -c[BYTES], --cap=[BYTES]          The base texture memory usage cap. The
                                        actual cap is this value plus the value
                                        computed from the --delta flag
      -d[PERCENT], --delta=[PERCENT]    Oscillate memory usage randomly within
                                        the band [cap - percent * cap, cap +
                                        percent * cap]
      -i[INTERVAL],
      --interval=[INTERVAL]             The number of frames between texture
                                        memory thrashes
      -w[WIDTH], --width=[WIDTH]        The width of a screen
      -h[HEIGHT], --height=[HEIGHT]     The height of a screen
      -s[COUNT], --screens=[COUNT]      The number of screens
      -v, --vertical                    Screens are vertically aligned (defaults
                                        to horizontal)
      --alloc-buffers                   Allocate a new source buffer for each
                                        mip upload
      --no-draw                         Do not draw any quads. (Textures are
                                        still created/deleted.)
```

**WARNING**: The develop branch is partially used for synchronization, and as
such it is subject to the occasional force push.
