#!/bin/bash
usage()
{
cat << EOF
usage: $(basename $0) options

Stage the stonehearth bits into some deployment directory

Require:
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
else
   MODULE_BUILD_TYPE=$BUILD_TYPE
fi
echo $BUILD_TYPE $MODULE_BUILD_TYPE

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
   LUA_ROOT=$STONEHEARTH_ROOT/modules/lua/package/lua
   cp -u $LUA_ROOT/solutions/$MODULE_BUILD_TYPE/lua-5.1.5.dll $OUTPUT_DIR

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
   CRASH_REPORTER_ROOT=$RADIANT_ROOT/stonehearth/build/source/lib/crash_reporter
   cp -u $CRASH_REPORTER_ROOT/$BUILD_TYPE/crash_reporter.exe $OUTPUT_DIR
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
   popd  > /dev/null
}

if [ ! -z $STAGE_DATA ]; then
   DATA_ROOT=$STONEHEARTH_ROOT/source/stonehearth_data

   pushd $DATA_ROOT  > /dev/null
   find . -maxdepth 1 -type f  \
      ! -name '*.log' \
      -print0 | xargs -0 cp -u --target-directory $OUTPUT_DIR
   popd  > /dev/null

   stage_data_dir horde
   stage_data_dir mods/radiant
   stage_data_dir mods/stonehearth
fi
