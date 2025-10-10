# Evolution-on

Evolution-on is an Evolution Email plugin to minimize the application to the system tray.

### Building

The build system is based on meson. To configure and build the project you may use something like:

```bash
$ meson setup build
$ meson compile -C build
$ meson install -C build
```

There are also two optional configuration options:
- `-Dinstall-schemas`: to install GSettings schema.
- `-Dinstall-gconf`: to install (deprecated) GConf schemas.

To see details run `meson configure`.
