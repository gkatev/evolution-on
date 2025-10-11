shared_library('liborg-gnome-evolution-on',
	[
		'tray.c',
		'sn.c',
		'sn.h',
		'ucount.c',
		'ucount.h',
		'properties.c',
		'properties.h'
	], name_prefix: '',
	dependencies: [
		evolutionshell,
		gtk,
		glib,
		dbusmenuglib,
		libgconfbridge,
		evolutionmail,
		libemailengine],
	install_mode: 'rwxr-xr-x',
	install: true,
	install_dir: plugindir)
