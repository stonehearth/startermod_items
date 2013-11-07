#!/bin/bash
if [ $# -ne 3 ]
   then
   echo "Usage: $(basename $0) <deployment-dir> <executable-dir> <data-dir>"
   exit 1
fi
echo

echo Creating deployment directory $1
echo
mkdir -p $1

echo Copying executable files from $2
echo
cd $2
find . -type f \
   ! -name '*.pdb' \
   ! -name '*.lib' \
   ! -name '*.exp' \
   ! -name '*.log' \
   ! -name '*.tlog' \
   ! -name '*.lastbuildstate' \
   ! -name 'lua.*' \
   ! -name 'sfml-*-d-2.dll' \
   ! -name 'cef.pak' \
   ! -name 'devtools_resources.pak' \
   -print0 | xargs -0 cp -u --parents --target-directory $1

echo Copying data files from $3
echo
cd $3
find . -type f  \
   ! -name '*.qmo' \
   ! -name '.gitignore' \
   ! -name 'debug.log' \
   ! -name 'backup.cmd' \
   -print0 | xargs -0 cp -u --parents --target-directory $1

echo Removing unused files from $1
echo
cd $1
set -x
rm -f debug.log
rm -rf mods/stonehearth_tests
rm -rf mods/stonehearth/_graveyard
rm -rf mods/radiant/js/external/ace
rm -rf mods/stonehearth_debugtools
