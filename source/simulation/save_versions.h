#ifndef _RADIANT_SIMULATION_SAVE_VERSIONS_H
#define _RADIANT_SIMULATION_SAVE_VERSIONS_H

#include "namespace.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

// Whenever someone does something to break save/load compatibility,
// they should:
//
// 1) Add a new enum to SaveVersion with a comment as to what first broke the
//    compatibility.
// 2) Move the CURRENT_SAVE_VERSION pointer to point to that version
//
enum SaveVersion {
   // First version!  This is the version we pushed to Steam Early access
   ALPHA_10_5 = 0, 

   // The current save version.  Should always point to the last item in
   // the list.                    
   CURRENT_SAVE_VERSION = ALPHA_10_5
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_SAVE_VERSIONS_H
