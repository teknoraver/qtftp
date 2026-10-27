# QTftpGui

QTftpGui is a small TFTP client and server written with Qt.

- The server shares a directory over TFTP.
- The client downloads (get) and uploads (put) files to a TFTP server.

The server listens on the standard TFTP port 69: on Linux binding it requires root privileges,
or the `CAP_NET_BIND_SERVICE` capability.

## Build

Requirements: CMake 3.19 or newer and Qt 6 (Widgets, Network).

```
cmake -B build
cmake --build build
```

## License

Copyright (C) 2015 Matteo Croce

QTftpGui is released under the GNU General Public License version 3 or later, see [COPYING](COPYING).
