#!/bin/bash
if [ $# -ne 3 ]
   then
   echo "Usage: $(basename $0) <deployment-dir> <executable-dir> <data-dir>"
   exit 1
fi
echo

# make deployment directory
echo Creating deployment directory
mkdir -p $1

# copy executable files
echo Copying executable files
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

# copy data files
echo Copying data files
cd $3
find . -type f  \
   ! -name '*.qmo' \
   ! -name '.gitignore' \
   ! -name 'debug.log' \
   ! -name 'backup.cmd' \
   -print0 | xargs -0 cp -u --parents --target-directory $1
