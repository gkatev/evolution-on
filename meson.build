project('evolution-tray','c',
	version: '3.24.4',
	meson_version: '>=0.58.0',
	license: 'GPL',
	default_options: ['prefix=/usr'])

add_project_arguments('-DHAVE_CONFIG_H', language:'c')
add_project_arguments('-I.', language:'c')

# Dependecies so far:
evolutionshell        = dependency('evolution-shell-3.0',       version: '>=3.6.4')
evolutionmail         = dependency('evolution-mail-3.0',        version: '>=3.6.0')
libemailengine        = dependency('libemail-engine',           version: '>=3.6.0')
gtk                   = dependency('gtk+-3.0',                  version: '>=3.6.0')
glib                  = dependency('glib-2.0')
dbusmenuglib          = dependency('dbusmenu-glib-0.4')

# Directories
LIB_INSTALL_DIR      = join_paths(get_option('prefix'), 'lib')
SHARE_INSTALL_PREFIX = join_paths(get_option('prefix'), 'share')
plugindir = evolutionshell.get_variable(pkgconfig: 'plugindir', default_value: '')

if plugindir == ''
	plugindir = join_paths(evolutionshell.get_variable(pkgconfig: 'privlibdir'), 'plugins')
endif

debugoption = get_option('debugbuild')
if debugoption == true
	add_project_arguments('-DDEBUG', language:'c')
endif

# GSettings starts here
schemasoption = get_option('install-schemas')
if schemasoption == true
	install_data('src/org.gnome.evolution.plugin.evolution-tray.gschema.xml',
		install_dir: get_option('datadir') / 'glib-2.0' / 'schemas' )
# we're not adding anything to compile any schemas. That is up to packager.  
endif

# Generate the config.h file
conf_data = configuration_data()

# Main project information
conf_data.set('PROJECT_NAME', meson.project_name())
conf_data.set('VERSION', meson.project_version())

# We dont want deprecated functions from evolution data server
conf_data.set('EDS_DISABLE_DEPRECATED', true)

# We dont use the old version anyway. If you do, good luck.
evoversion = evolutionshell.version()
evoversion = evoversion.replace('.','')
evoversion_int = evoversion.to_int()
evoversion_int = evoversion_int * 10

conf_data.set('EVOLUTION_VERSION', evoversion_int)

conf_data.set('PLUGIN_INSTALL_DIR', plugindir)
conf_data.set('GETTEXT_PACKAGE', meson.project_name())
conf_data.set('LOCALEDIR', join_paths(SHARE_INSTALL_PREFIX, 'locale'))

configure_file(
	input: 'src/org-gnome-evolution-tray.eplug.in',
	output: 'org-gnome-evolution-tray.eplug',
	configuration: conf_data,
	install: true,
	install_dir: plugindir,
	install_mode: 'rw-r--r--'
)

configure_file(
	output: 'config.h',
	configuration: conf_data, 
)

subdir('src')
subdir('po')
