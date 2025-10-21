# Evolution Tray

Evolution Tray is an Evolution Email plugin to minimize the application to the
system tray.

This project is a fork of Evolution-on. The main differentiation point is the
implementation of the tray icon using `StatusNotifierItem` instead of the
deprecated and now poorly working `GtkStatusIcon`.

Unlike Evolution On, Evolution Tray only handles tray-related functionality.
Other features like mail notifications are offered by the upstream
`mail-notification` plugin.

### Building/Installing

#### AUR

Coming soon (?)

#### From Source

The build system is based on meson. To configure and build the project you
could use something like:

```bash
$ meson setup build -Dinstall-schemas=true
$ meson compile -C build
$ meson install -C build
```

Optional setup options:
- `-Dinstall-schemas=true`: Install GSettings schema
- `-Ddebugbuild=true`: Debug build

FYI: The first time you install the GSettings schema, you might then also need
to compile the schemas system-wide, something that the build script does not
handle. The approach may vary depending on the platform.

On Arch Linux: \
`# glib-compile-schemas /usr/share/glib-2.0/schemas`
