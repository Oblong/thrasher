A texture memory thrasher.

This project is built using [meson](http://mesonbuild.com).

Dependencies (Ubuntu 16.04)
-
```
sudo apt-get install libglfw3-dev
```

Meson is a young project, and Ubuntu's version lacks many features. The
following is recommended:
```
sudo apt-get install python-pip3
pip3 install meson
```

But this may work: `sudo apt-get install meson`

Build
-
```
git submodule update --init
meson build --buildtype=release
ninja -C build
```

Example Invocation
-
```
build/thrash -w 1920 -h 1080 -c 3 -t 1000 -m 1000000000
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
      -m[BYTES], --memory-cap=[BYTES]   The base texture memory usage cap. The
                                        actual cap is this value plus the value
                                        computed from the --delta flag
      -d[PERCENT], --delta=[PERCENT]    Oscillate memory usage randomly within
                                        the band [memory-cap - delta *
                                        memory-cap, memory-cap + delta *
                                        memory-cap]
      -i[INTERVAL],
      --interval=[INTERVAL]             The number of frames between texture
                                        memory thrashes
      -w[WIDTH], --width=[WIDTH]        The width of a screen
      -h[HEIGHT], --height=[HEIGHT]     The height of a screen
      -c[COUNT], --columns=[COUNT]      The number of screen columns
      -r[COUNT], --rows=[COUNT]         The number of screen rows
      --alloc-buffers                   Allocate a new source buffer for each
                                        mip upload
      --no-draw                         Do not draw any quads. (Textures are
                                        still created/deleted.)
```

**WARNING**: The develop branch is partially used for synchronization, and as
such it is subject to the occasional force push.
