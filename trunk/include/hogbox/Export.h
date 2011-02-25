#pragma once

#include<hogbox/Config.h>


// disable VisualStudio warnings 
#if defined(_MSC_VER) && defined(HOGBOX_DISABLE_MSVC_WARNINGS)
    #pragma warning( disable : 4244 )
    #pragma warning( disable : 4251 )
    #pragma warning( disable : 4275 )
    #pragma warning( disable : 4512 )
    #pragma warning( disable : 4267 )
    #pragma warning( disable : 4702 )
    #pragma warning( disable : 4511 )
#endif

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
    #  if defined( HOGBOX_LIBRARY_STATIC )
    #    define HOGBOX_EXPORT
    #  elif defined( HOGBOX_LIBRARY )
    #    define HOGBOX_EXPORT   __declspec(dllexport)
    #  else
    #    define HOGBOX_EXPORT   __declspec(dllimport)
    #  endif
#else
    #  define HOGBOX_EXPORT
#endif

// set up define for whether member templates are supported by VisualStudio compilers.
#ifdef _MSC_VER
# if (_MSC_VER >= 1300)
#  define __STL_MEMBER_TEMPLATES
# endif
#endif

/* Define NULL pointer value */

#ifndef NULL
    #ifdef  __cplusplus
        #define NULL    0
    #else
        #define NULL    ((void *)0)
    #endif
#endif
