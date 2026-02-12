// This file contains all other files in the library

#include "ssb.c"
#include "stx.c"
#include "sth2.c"
#include "wav.c"
#include "alr.c"

// These files depend on external libraries
#if defined(PD_COMMON_HAVE_MINIAUDIO_IBXM)
#include "ibxm_reader.c"
#include "miniaudio_ibxm.c"
#include "miniaudio_stx.c"
#endif
