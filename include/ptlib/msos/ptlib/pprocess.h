/*
 * $Id: pprocess.h,v 1.6 1995/04/25 11:17:11 robertj Exp $
 *
 * Portable Windows Library
 *
 * User Interface Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: pprocess.h,v $
 * Revision 1.6  1995/04/25 11:17:11  robertj
 * Fixes for DLL use in WIN32.
 *
 * Revision 1.5  1995/03/12 04:59:57  robertj
 * Re-organisation of DOS/WIN16 and WIN32 platforms to maximise common code.
 * Used built-in equate for WIN32 API (_WIN32).
 *
 * Revision 1.4  1994/07/27  06:00:10  robertj
 * Backup
 *
 * Revision 1.3  1994/07/21  12:35:18  robertj
 * *** empty log message ***
 *
 * Revision 1.2  1994/07/02  03:18:09  robertj
 * Prevent WinMain in pure MSDOS versions.
 *
 * Revision 1.1  1994/06/25  12:13:01  robertj
 * Initial revision
 *
 */


#ifndef _PPROCESS


///////////////////////////////////////////////////////////////////////////////
// PProcess

#include "../../common/process.h"
#if defined(_WINDOWS) || defined(_WIN32)
  friend int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
#endif
};


#if defined(_WIN32) || !defined(_WINDLL)

#if defined(_WIN32) && defined(_WINDLL)
extern __declspec(dllexport) PProcess * PProcessInstance;
#else
extern PProcess * PSTATIC PProcessInstance;
#endif

inline PProcess::PProcess()
  { PProcessInstance = this; }

inline PProcess * PProcess::Current()
  { return PProcessInstance; }

#endif


#endif
