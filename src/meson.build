shared_library('liborg-gnome-evolution-on',
	[
		'tray.c',
		'properties.c',
		'properties.h',
		'sn.c',
		'sn.h'
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
