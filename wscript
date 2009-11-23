#! /usr/bin/env python
# encoding: utf-8
# pktfag, 2009

import sys

APPNAME = 'pidgin-juick-plugin'

srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('intltool')

def configure(conf):
	is_win32=sys.platform=='win32'

	conf.check_tool('gcc')
	# we don't require intltool on Windows (it would require Perl) though it works well
	conf.define('ENABLE_NLS', 1)
	try:
		conf.check_tool('intltool')
	except:
		conf.undefine('ENABLE_NLS')
		pass

	conf.check_cfg(package='purple',
			args='--cflags --libs',
			uselib_store='purple',
			mandatory=True)
	conf.check_cfg(package='pidgin',
			args='--cflags --libs',
			uselib_store='pidgin',
			mandatory=True)

	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')

	if is_win32:
		# on Windows LOCALEDIR define in purple win32dep.h
		conf.undefine('DATADIR')
		conf.undefine('LOCALEDIR')
	conf.define('GETTEXT_PACKAGE', APPNAME, 1)
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

