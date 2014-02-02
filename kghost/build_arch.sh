#!/bin/sh
# Script to get proper architecture for audizzle
#

opSys=`uname -s`
archName=`arch`

if [ "$archName" = "unknown" ]; then
	$archName='uname -p'
fi

if echo "$opSys" | egrep "Darwin" > /dev/null; then
	if echo "$HOSTTYPE" | egrep "i.86" > /dev/null; then
		echo "MAC_X86_32"
	elif echo "$HOSTTYPE" | egrep "x86_64" > /dev/null; then
		echo "MAC_X86_64"
	else
		echo "MAC_X86_32"
	fi
elif echo "$opSys" | egrep "Linux" > /dev/null; then
	if echo "$archName" | egrep "i.86" > /dev/null; then
		echo "LINUX_X86_32"
	elif echo "$archName" | egrep "x86_64" > /dev/null; then
		echo "LINUX_X86_64"
	else
		echo "LINUX_X86_32"
	fi
elif echo "$opSys" | egrep "CYGWIN" > /dev/null; then
	if echo "$archName" | egrep "i.86" > /dev/null; then
		echo "WIN_X86_32"
	elif echo "$archName" | egrep "x86_64" > /dev/null; then
		echo "WIN_X86_64"
	else
		echo "WIN_X86_32"
	fi
elif echo "$opSys" | egrep "MINGW" > /dev/null; then
	if echo "$archName" | egrep "i.86" > /dev/null; then
		echo "WIN_X86_32"
	elif echo "$archName" | egrep "x86_64" > /dev/null; then
		echo "WIN_X86_64"
	else
		echo "WIN_X86_32"
	fi
fi
