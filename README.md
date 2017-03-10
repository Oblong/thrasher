Some OpenGL forensic/diagnostic/debugging utilities for mezzanine.

For now, this project is using the [tup](http://gittup.org/tup/) build system.

Dependencies (Ubuntu)
-
```
sudo apt-get install libglfw3-dev
sudo apt-add-repository 'deb http://ppa.launchpad.net/anatol/tup/ubuntu precise main'
sudo apt-get update
sudo apt-get install tup
```

Build
-
```
git submodule update --init
tup init
tup upd
```

Currently this project contains a tool for instrumentation via LD_PRELOAD and a
texture memory thrasher.

Memory Thrasher
-

```
  ./thrash {OPTIONS}

    A texture memory thrasher

  OPTIONS:

      --help                            Display this message
      -t[BYTES], --texture-size=[BYTES] The maximum texture size in bytes
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
      -a, --alloc_buffers               Allocate a new source buffer for each
                                        mip upload
```

**WARNING**: The develop branch is partially used for synchronization, and as
such it is subject to the occasional force push.
