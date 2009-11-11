#! /usr/bin/env python
# encoding: utf-8
# pktfag, 2009

srcdir = '.'
blddir = 'build'

def set_options(opt):
	pass
def configure(conf):
	conf.check_tool('gcc')
	conf.check_tool('intltool')

	conf.check_cfg(package='purple',
			args='--cflags --libs',
			uselib_store='purple')
	conf.check_cfg(package='pidgin',
			args='--cflags --libs',
			uselib_store='pidgin')

def build(bld):
	if bld.env.INTLTOOL:
		bld.add_subdirs('po')

	bld.new_task_gen(
			features = 'cc cshlib',
			source = 'src/juick.c',
			target = 'juick',
			uselib = 'pidgin purple'
			)
	pass

