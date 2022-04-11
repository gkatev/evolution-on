project('evolution-on','c',
	version: '3.24.3',
	meson_version: '>=0.58.0',
	license: 'GPL',
	default_options: ['prefix=/usr'])

add_project_arguments('-DHAVE_CONFIG_H', language:'c')
add_project_arguments('-I.', language:'c')

# Dependecies so far:
evolutionshell        = dependency('evolution-shell-3.0',       version: '>=3.6.4')
gtk                   = dependency('gtk+-3.0',                  version: '>=3.6.0')
glib                  = dependency('glib-2.0')
libnotify             = dependency('libnotify',                 version: '>=0.5.1', required : false)
libappindicator       = dependency('appindicator-0.1',                              required : false)
libgconfbridge        = dependency('libgconf-bridge',                               required : false)
evolutionmail         = dependency('evolution-mail-3.0',        version: '>=3.6.0')
libemailengine        = dependency('libemail-engine',           version: '>=3.6.0')

# Directories
LIB_INSTALL_DIR      = join_paths(get_option('prefix'), 'lib')
SHARE_INSTALL_PREFIX = join_paths(get_option('prefix'), 'share')
plugindir = evolutionshell.get_variable(pkgconfig: 'plugindir')

if plugindir.version_compare('')
	plugindir = join_paths(evolutionshell.get_variable(pkgconfig: 'privlibdir'), 'plugins')
endif

debugoption = get_option('debugbuild')
if debugoption == true
	add_project_arguments('-DDEBUG', language:'c')
endif

# GSettings starts here
schemasoption = get_option('install-schemas')
if schemasoption == true
	install_data('src/org.gnome.evolution.plugin.evolution-on.gschema.xml', install_dir: get_option('datadir') / 'glib-2.0' / 'schemas' )
# we're not adding anything to compile any schemas. That is up to packager.  
endif
# And we continue witg GConf. GConf is dead. So we shouldn't really use it anyway.
gconfoption = get_option('install-gconf')
if gconfoption == true
	install_data('src/evolution-on.convert', install_dir: get_option('datadir') / 'GConf' / 'gsettings' )
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
evoversion_int = evoversion.to_int(evoversion)
evoversion_int = evoversion_int * 10

conf_data.set('EVOLUTION_VERSION', evoversion_int)

conf_data.set('PLUGIN_INSTALL_DIR', plugindir)
conf_data.set('GETTEXT_PACKAGE', meson.project_name())
conf_data.set('LOCALEDIR', join_paths(SHARE_INSTALL_PREFIX, 'locale'))

if libappindicator.found() and libnotify.found()
# If we got both, we prefer libappindicator as that is what probably user wants to use anyway
	conf_data.set('HAVE_LIBAPPINDICATOR', true)
else
# Otherwise just use libnotify
	conf_data.set('HAVE_LIBNOTIFY', true)
	libnotifyversion = libnotify.version()
	libnotifyversion = libnotifyversion.replace ('.','')
	libnotifyversion_int = libnotifyversion.to_int() 
	libnotifyversion_int = libnotifyversion_int * 100
	conf_data.set('LIBNOTIFY_VERSION', libnotifyversion_int)

endif

if libnotify.found() and not libappindicator.found()
	conf_data.set('HAVE_LIBNOTIFY', true)
	libnotifyversion = libnotify.version()
	libnotifyversion = libnotifyversion.replace ('.','')
	libnotifyversion_int = libnotifyversion.to_int() 
	libnotifyversion_int = libnotifyversion_int * 100
	conf_data.set('LIBNOTIFY_VERSION', libnotifyversion_int)
endif

if libgconfbridge.found()
	conf_data.set('HAVE_LIBGCONFBRIDGE', true)
endif

configure_file(
	input: 'src/org-gnome-evolution-on.eplug.in',
	output: 'org-gnome-evolution-on.eplug',
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