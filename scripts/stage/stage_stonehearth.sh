#!/bin/bash
usage()
{
cat << EOF
usage: $(basename $0) options

Stage the stonehearth bits into some deployment directory

Required:
   --o      the directory to write the files to
   --t      the build type to stage (e.g. Debug)

Optional:
   --c      clean the contents of the output directory
   --a      stage everything
   --b      stage dependencies require to run stonehearth
   --d      stage stonehearth run data
   --s      stage the stonehearth binary itself
EOF
}

STONEHEARTH_ROOT=`pwd`
OUTPUT_DIR=
BUILD_TYPE=
STAGE_SELF=
STAGE_DATA=
STAGE_BIN=
CLEAN_OUTPUT=

while getopts "o:t:cabds" OPTION; do
   case $OPTION in
      o)
         OUTPUT_DIR=$STONEHEARTH_ROOT/$OPTARG
         ;;
      t)
         BUILD_TYPE=$OPTARG
         ;;
      a) 
         STAGE_BIN=1
         STAGE_SELF=1
         STAGE_DATA=1
         ;;
      c)
         CLEAN_OUTPUT=1
         ;;
      b)
         STAGE_BIN=1
         ;;
      d)
         STAGE_DATA=1
         ;;
      s) 
         STAGE_SELF=1
         ;;
      ?)
         usage
         exit 0
   esac
done

if [ $BUILD_TYPE == RelWithDebInfo ]; then
   MODULE_BUILD_TYPE=Release
   MODULE_BUILD_SUFFIX=
else
   MODULE_BUILD_TYPE=$BUILD_TYPE
   MODULE_BUILD_SUFFIX=d
fi

if [ -z $OUTPUT_DIR ] || [ -z $BUILD_TYPE ]; then
   usage
   exit 1
fi

if [ ! -z $CLEAN_OUTPUT ]; then
   echo Cleaning deployment directory $OUTPUT_DIR
   rm -rf $OUTPUT_DIR/*
fi

if [ ! -d $OUTPUT_DIR ]; then
   echo Creating deployment directory $OUTPUT_DIR
   mkdir -p $OUTPUT_DIR
fi

if [ ! -z $STAGE_SELF ]; then
   echo Copying stonehearth binaries
   BUILD_ROOT=$STONEHEARTH_ROOT/build/source/stonehearth/$BUILD_TYPE
   cp -u $BUILD_ROOT/Stonehearth.exe $OUTPUT_DIR    
fi

if [ ! -z $STAGE_BIN ]; then
   echo Copying lua binaries

   # vanilla lua. no jit.  no fun.
   LUA_ROOT=$STONEHEARTH_ROOT/modules/lua/package/lua
   cp -u $LUA_ROOT/solutions/$MODULE_BUILD_TYPE/lua-5.1.5.dll $OUTPUT_DIR

   # luajit up in here! party time!!
   # LUA_ROOT=$STONEHEARTH_ROOT/modules/luajit/src
   # cp -u $LUA_ROOT/lua51${MODULE_BUILD_SUFFIX}.dll $OUTPUT_DIR

   echo Copying chromium embedded
   CHROMIUM_ROOT=$STONEHEARTH_ROOT/modules/chromium-embedded/package/cef_binary_3.1547.1412_windows32
   cp -u -r $CHROMIUM_ROOT/resources/locales $OUTPUT_DIR/locales
   cp -u $CHROMIUM_ROOT/Release/libcef.dll $OUTPUT_DIR
   cp -u $CHROMIUM_ROOT/Release/icudt.dll $OUTPUT_DIR

   echo Copying sfml 
   SFML_ROOT=$STONEHEARTH_ROOT/modules/sfml
   SFML_BUILD_ROOT=$SFML_ROOT/build/lib/$MODULE_BUILD_TYPE
   SFML_EXTLIB_ROOT=$SFML_ROOT/package/SFML-2.1/extlibs/bin/x86
   if [ $BUILD_TYPE == Debug ]; then
      SUFFIX=-d
   else
      SUFFIX=
   fi
   cp -u $SFML_BUILD_ROOT/sfml-audio${SUFFIX}-2.dll $OUTPUT_DIR
   cp -u $SFML_BUILD_ROOT/sfml-system${SUFFIX}-2.dll $OUTPUT_DIR
   cp -u $SFML_EXTLIB_ROOT/openal32.dll $OUTPUT_DIR
   cp -u $SFML_EXTLIB_ROOT/libsndfile-1.dll $OUTPUT_DIR

   echo Copying crash reporter
   CRASH_REPORTER_ROOT=$STONEHEARTH_ROOT/build/source/lib/crash_reporter
   cp -u $CRASH_REPORTER_ROOT/server/$BUILD_TYPE/crash_reporter.exe $OUTPUT_DIR
fi

function stage_data_dir
{
   # $1 - the name of the mod to stage
   echo Copying data files for $1
   mkdir -p $OUTPUT_DIR/$1
   pushd $DATA_ROOT/$1 > /dev/null
   find . -type f  \
      ! -name '*.qmo' \
      -print0 | xargs -0 cp -u --parents --target-directory $OUTPUT_DIR/$1
   popd > /dev/null
}

function compile_lua_and_package_module
{
   # $1 - the name of the mod to stage
   stage_data_dir $1

   echo Compiling lua and packaging module in $1
   MOD_NAME=${1##*/}
   pushd $OUTPUT_DIR/$1/.. > /dev/null
   for infile in $(find $MOD_NAME -type f -name '*.lua'); do
     OUTFILE=${infile}c
     $LUA_BIN_ROOT/luac.exe -o $OUTFILE $infile
     rm -f $infile
   done

   # zip the package
   # no silent mode for 7-zip, could save output to file and cat file if [ $? -ne 0 ] 
   rm -f $MOD_NAME.smod
   7za a -r -tzip -mx=9 $MOD_NAME.smod $MOD_NAME/'*' > /dev/null

   rm -rf $MOD_NAME
   popd > /dev/null
}

if [ ! -z $STAGE_DATA ]; then
   DATA_ROOT=$STONEHEARTH_ROOT/source/stonehearth_data
   LUA_BIN_ROOT=$STONEHEARTH_ROOT/modules/lua/package/lua/solutions/release

   pushd $DATA_ROOT > /dev/null
   find . -maxdepth 1 -type f  \
      ! -name '*.log' \
      -print0 | xargs -0 cp -u --target-directory $OUTPUT_DIR
   popd > /dev/null

   stage_data_dir horde

   compile_lua_and_package_module mods/radiant
   compile_lua_and_package_module mods/stonehearth
fi
