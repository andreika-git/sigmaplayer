#ifndef SP_VERSION_H
#define SP_VERSION_H


#ifdef SP_PLAYER_TECHNOSONIC
    #include "version-MP.h"
#endif

#ifdef SP_PLAYER_DREAMX108
    #include "version-DX.h"
#endif

#ifdef SP_PLAYER_MECOTEK
    #include "version-MT.h"
#endif

#ifdef WIN32
    #include "version-WIN.h"
#endif


#endif // of SP_VERSION_H
