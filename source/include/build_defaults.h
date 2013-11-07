#ifndef _BUILD_DEFAULTS_H
#define _BUILD_DEFAULTS_H

#ifndef _BUILD_NUMBER_H
# error "Do not include this file!  Include build_number.h"
#endif

/*
 * This is not the file you're probably looking for.  It exists only to get
 * the build to compile in developer builds.  The actual file used by official
 * builds (which probably has the defines your looking for), is generated by
 * the official build script and placed into the build directory (which is
 * included before this file in the include path).
 *
 * Modifying anything in here is almost certainly a mistake.  You have been
 * warned!
 */

#define PRODUCT_IDENTIFIER          "stonehearth"
#define PRODUCT_NAME                "Stonehearth Development"
#define PRODUCT_MAJOR_VERSION       1
#define PRODUCT_MINOR_VERSION       2
#define PRODUCT_PATCH_VERSION       3
#define PRODUCT_BUILD_NUMBER        45678
#define PRODUCT_REVISION            "#revision#"
#define PRODUCT_BRANCH              "#branch#"
#define PRODUCT_VERSION_STR         "1.2.3"
#define PRODUCT_FILE_VERSION_STR    "1.2.3.45679"


/* These route to the Stonehearth Dev bucket in Game Analytics.  Do not change
 * them!  Ever!  The build system will override them if necessary */
#define GAME_ANALYTICS_GAME_KEY        "2b6cc12b9457de0ae969e0d9f8b04291"
#define GAME_ANALYTICS_SECRET_KEY      "70904f041d9e579c3d34f40cdb5bc0c16ad0c09a"
#define GAME_ANALYTICS_DATA_API_KEY    "79fbff531bea1d30436ef6ec63a2ab71ecf6f62"

#endif // _BUILD_DEFAULTS_H
