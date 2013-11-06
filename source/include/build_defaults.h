#ifndef _BUILD_DEFAULTS_H
#define _BUILD_DEFAULTS_H

/*
 * Do not include this file.  Include build_number.h.  Thanks
 */

#ifndef _BUILD_NUMBER_H
# error "Do not include this file!  Include build_number.h
#endif

/* These route to the Stonehearth Dev bucket in Game Analytics.  Do not change
 * them!  Ever!  The build system will override them if necessary */
#define GAME_ANALYTICS_GAME_KEY        "2b6cc12b9457de0ae969e0d9f8b04291"
#define GAME_ANALYTICS_SECRET_KEY      "70904f041d9e579c3d34f40cdb5bc0c16ad0c09a"
#define GAME_ANALYTICS_DATA_API_KEY    "79fbff531bea1d30436ef6ec63a2ab71ecf6f62"

#endif // _BUILD_DEFAULTS_H
