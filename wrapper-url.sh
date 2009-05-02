#!/bin/bash

case $1 in
    xmpp:*)
	url=`echo $1 | sed -e 's/#/%23/'`
	purple-url-handler "$url"
	;;
    *)
	if [ `which firefox 2>/dev/null` ]; then
	    firefox "$1"
	elif [ `which opera 2>/dev/null` ]; then
	    opera "$1"
	fi
esac
