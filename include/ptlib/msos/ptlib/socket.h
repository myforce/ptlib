/*
 * $Id: socket.h,v 1.12 1996/10/08 13:05:01 robertj Exp $
 *
 * Portable Windows Library
 *
 * Socket Class Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: socket.h,v $
 * Revision 1.12  1996/10/08 13:05:01  robertj
 * More IPX support.
 *
 * Revision 1.11  1996/09/14 13:09:46  robertj
 * Major upgrade:
 *   rearranged sockets to help support IPX.
 *   added indirect channel class and moved all protocols to descend from it,
 *   separating the protocol from the low level byte transport.
 *
 * Revision 1.10  1996/08/08 10:09:14  robertj
 * Directory structure changes for common files.
 *
 * Revision 1.9  1996/07/27 04:08:58  robertj
 * Created static version of ConvertOSError().
 *
 * Revision 1.8  1996/03/31 09:11:40  robertj
 * Fixed major performance problem in timeout read/write to sockets.
 *
 * Revision 1.7  1995/12/10 11:49:43  robertj
 * Numerous fixes for sockets.
 *
 * Revision 1.6  1995/03/12 05:00:01  robertj
 * Re-organisation of DOS/WIN16 and WIN32 platforms to maximise common code.
 * Used built-in equate for WIN32 API (_WIN32).
 *
 * Revision 1.5  1995/01/02  12:16:22  robertj
 * Moved constructor to platform dependent code.
 *
 * Revision 1.4  1994/12/12  10:10:17  robertj
 * Changed so can compile if no winsock available.
 *
 * Revision 1.3  1994/10/30  11:24:22  robertj
 * Fixed DOS version of header.
 *
 * Revision 1.2  1994/10/23  05:36:51  robertj
 * Sockets implementation.
 *
 * Revision 1.1  1994/08/22  00:18:02  robertj
 * Initial revision
 *
 * Revision 1.1  1994/07/27  06:00:10  robertj
 * Initial revision
 *
 */

#ifndef _PSOCKET


#if (defined(_WINDOWS) && defined(PHAS_WINSOCK)) || defined(_WIN32)

#include <winsock.h>

#define PIPX

#endif


#include "../../common/ptlib/socket.h"
  public:
    ~PSocket();
      // close a socket

    virtual BOOL Read(void * buf, PINDEX len);
    virtual BOOL Write(const void * buf, PINDEX len);
    virtual BOOL Close();

  protected:
    BOOL ConvertOSError(int error);
    static BOOL ConvertOSError(int error, Errors & lastError, int & osError);

  private:
#ifdef PHAS_WINSOCK
    static BOOL WinSockStarted;
#endif
};


PDECLARE_CLASS(PWinSock, PSocket)
// Must be one and one only instance of this class, and it must be static!.
  public:
    PWinSock();
    ~PWinSock();
  private:
    virtual BOOL OpenSocket();
    virtual const char * GetProtocolName() const;
};


#endif


// End Of File ///////////////////////////////////////////////////////////////
