/*
 * $Id: pprocess.h,v 1.12 1998/05/30 13:30:44 robertj Exp $
 *
 * Portable Windows Library
 *
 * User Interface Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: pprocess.h,v $
 * Revision 1.12  1998/05/30 13:30:44  robertj
 * Fixed shutdown problems with PConfig caching.
 *
 * Revision 1.11  1998/03/29 10:42:52  craigs
 * Made PConfig thread safe
 *
 * Revision 1.10  1998/03/26 04:55:53  robertj
 * Added PMutex and PSyncPoint
 *
 * Revision 1.9  1998/01/04 08:13:32  craigs
 * Removed extern reference to PProcessInstance
 *
 * Revision 1.8  1998/01/03 23:06:32  craigs
 * Added PThread support
 *
 * Revision 1.7  1996/10/31 10:28:38  craigs
 * Removed PXOnSigxxx routines
 *
 * Revision 1.6  1996/09/21 05:42:12  craigs
 * Changes for new common files, PConfig changes and signal handling
 *
 * Revision 1.5  1996/08/03 12:08:19  craigs
 * Changed for new common directories
 *
 * Revision 1.4  1996/06/10 11:03:23  craigs
 * Changed include name
 *
 * Revision 1.3  1996/04/15 10:50:48  craigs
 * Last revision prior to release of MibMaster
 *
 * Revision 1.2  1996/01/26 11:06:31  craigs
 * Added signal handlers
 *
 * Revision 1.1  1995/01/23 18:43:27  craigs
 * Initial revision
 *
 * Revision 1.1  1994/04/12  08:31:05  robertj
 * Initial revision
 *
 */

#ifndef _PPROCESS

#pragma interface

#include <syncpoint.h>

PDICTIONARY(PXFdDict,    POrdinalKey, PThread);


///////////////////////////////////////////////////////////////////////////////
// PProcess

#include "../../common/ptlib/pprocess.h"
  public:
    friend class PApplication;
    friend class PServiceProcess;
    friend void PXSignalHandler(int);
    friend class HouseKeepingThread;

    ~PProcess();

    PDirectory PXGetHomeDir ();
    char ** PXGetEnvp() const;

    friend void PXSigHandler(int);
    virtual void PXOnSignal(int);
    virtual void PXOnAsyncSignal(int);

    static void PXShowSystemWarning(PINDEX code);
    static void PXShowSystemWarning(PINDEX code, const PString & str);

  protected:
    void         CommonConstruct();
    void         CommonDestruct();

    void         PXCheckSignals();
    virtual void _PXShowSystemWarning(PINDEX code, const PString & str);
    int pxSignals;

  protected:
    void CreateConfigFilesDictionary();
    PAbstractDictionary * configFiles;


#ifndef P_PTHREADS
  public:
    void PXAbortIOBlock(int fd);
  protected:
    PXFdDict     ioBlocks[3];
#else
  friend void * PXHouseKeepingThread(void *);

  public:
    void SignalTimerChange();

  protected:
    PDICTIONARY(ThreadDict, POrdinalKey, PThread);
    ThreadDict activeThreads;
    PMutex     threadMutex;
    PSyncPoint timerChangeSemaphore;
    PThread * housekeepingThread;
#endif
};

#endif
