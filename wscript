#! /usr/bin/env python
# encoding: utf-8
# pktfag, 2009

APPNAME = 'pidgin-juick-plugin'

srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('intltool')

def configure(conf):
	conf.check_tool('gcc')
	# we don't require intltool on Windows (it would require Perl) though it works well
	try:
		conf.check_tool('intltool')
	except:
		pass

	conf.check_cfg(package='purple',
			args='--cflags --libs',
			uselib_store='purple')
	conf.check_cfg(package='pidgin',
			args='--cflags --libs',
			uselib_store='pidgin')

	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')

	print conf.env.LIBDIR 
	conf.define('GETTEXT_PACKAGE', APPNAME, 1)
	conf.define('PACKAGE', APPNAME, 1)
	conf.define('ENABLE_NLS', 1)
	conf.write_config_header('config.h')

def build(bld):
	bld.env.LIBDIR = '${PREFIX}/lib/pidgin'
	if bld.env.INTLTOOL:
		bld.new_task_gen(
				features = 'intltool_po',
				podir = 'po',
				install_path = '${LOCALEDIR}',
				appname	= APPNAME
				)

	bld.new_task_gen(
			features = 'cc cshlib',
			source = 'src/juick.c',
			includes = '.',
			target = APPNAME,
			uselib = 'pidgin purple'
			)
	pass

