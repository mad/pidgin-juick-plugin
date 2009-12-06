#! /usr/bin/env python
# encoding: utf-8
# pktfag, 2009

import os, sys
import Options

APPNAME = 'pidgin-juick-plugin'
VERSION = '0.3.2'

srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('compiler_cc')
	opt.tool_options('intltool')
	group = opt.add_option_group ('Install', '')
	group.add_option('--for-nsis',
			action='store_true',
			default=False,
			help='Install for Windows NSIS (work with option -w or for Windows platform)',
			dest='nsis')
	group = opt.add_option_group ('Windows', '')
	group.add_option('-w',
			action='store_true',
			default=False,
			help='Configure and crosscompile for Windows',
			dest='win32')

def configure(conf):
	is_win32=sys.platform=='win32'

	if Options.options.win32:
		# create the second environment, set the variant and set its name
		env = conf.env.copy()
		env.set_variant('win32')
		conf.set_env_name('win32', env)
		# call the debug environment
		conf.setenv('win32')

	conf.check_tool('compiler_cc')
	#conf.check_tool('gcc')
	# we don't require intltool on Windows (it would require Perl) though it works well
	conf.define('ENABLE_NLS', 1)
	try:
		conf.check_tool('intltool')
	except:
		conf.undefine('ENABLE_NLS')

	conf.check_cfg(package='purple',
			args='--cflags --libs',
			uselib_store='purple',
			mandatory=True)
	conf.check_cfg(package='pidgin',
			args='--cflags --libs',
			uselib_store='pidgin',
			mandatory=True)

	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')

	if is_win32 or Options.options.win32:
		# on Windows LOCALEDIR define in purple win32dep.h
		conf.undefine('DATADIR')
		conf.undefine('LOCALEDIR')
	conf.define('GETTEXT_PACKAGE', APPNAME, 1)
	conf.write_config_header('config.h')

def build(bld):
	is_win32=sys.platform=='win32' or Options.options.win32
	if Options.options.win32:
		variant_name='win32'
	else:
		variant_name='default'
	envx = bld.env_of_name(variant_name)

	if is_win32:
		envx['LOCALEDIR'] = os.path.join(envx['PREFIX'], 'locale')
		if Options.options.nsis:
			envx['BINDIR'] = envx['PREFIX']
			envx['LIBDIR'] = envx['PREFIX']
		else:
			envx['BINDIR'] = os.path.join(envx['PREFIX'], 'plugins')
			envx['LIBDIR'] = os.path.join(envx['PREFIX'], 'plugins')
	else:
		envx['LIBDIR'] = os.path.join(envx['PREFIX'], 'lib', 'pidgin')
	if envx['INTLTOOL']:
		bld.new_task_gen(
				features = 'intltool_po',
				podir = 'po',
				appname	= APPNAME,
				env = envx.copy()
				)

	bld.new_task_gen(
			features = 'cc cshlib',
			source = 'src/juick.c',
			includes = '.',
			target = APPNAME,
			uselib = 'pidgin purple',
			env = envx.copy()
			)
	if is_win32 and Options.options.nsis:
		bld.install_as(envx['PREFIX'] + '/ChangeLog.txt', 'ChangeLog')
		bld.install_as(envx['PREFIX'] + '/COPYING.txt', 'COPYING')
		bld.install_files(envx['PREFIX'], 'packaging/windows/pidgin-juick-plugin.nsi')

def dist():
	import Utils, Scripting
	if sys.hexversion >= 0x2060000:
		import subprocess as pproc
	else:
		import pproc
	from Logs import info
	import tarfile, shutil
	try:
		tmp_folder = APPNAME + '-' + VERSION
		arch_name = tmp_folder+'.tar.'+Scripting.g_gz

		# remove the previous dir
		try:
			shutil.rmtree(tmp_folder)
		except (OSError, IOError):
			pass

		# remove the previous archive
		try:
			os.remove(arch_name)
		except (OSError, IOError):
			pass

		os.makedirs(tmp_folder)
		p1 = pproc.Popen(['git', 'archive', 'HEAD'], stdout=pproc.PIPE)
		p2 = pproc.Popen(['tar', '-C', tmp_folder, '-xf', '-'], stdin=p1.stdout)

		tar = tarfile.open(arch_name, 'w:' + Scripting.g_gz)
		tar.add(tmp_folder)
		tar.close()

		try: from hashlib import sha1 as sha
		except ImportError: from sha import sha
		try:
			digest = " (sha=%r)" % sha(Utils.readf(arch_name)).hexdigest()
		except:
			digest = ''

		info('New archive created: %s%s' % (arch_name, digest))
		if os.path.exists(tmp_folder): shutil.rmtree(tmp_folder)
	except:
		Scripting.dist(APPNAME, VERSION)

