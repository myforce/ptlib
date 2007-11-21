/*
 * assert.cxx
 *
 * Assert function implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#include <ptlib.h>

#include <ctype.h>
#include <signal.h>
#include <stdexcept>
#include <ptlib/pprocess.h>

#ifndef __BEOS__
#ifndef P_VXWORKS

static BOOL PAssertAction(int c, const char * msg)
{
  switch (c) {
    case 'a' :
    case 'A' :
      PError << "\nAborting.\n";
      _exit(1);
      break;

#if P_EXCEPTIONS
    case 't' :
    case 'T' :
      PError << "\nThrowing exception\n";
      throw std::runtime_error(msg);
      return TRUE;
#endif
        
#ifdef _DEBUG
    case 'd' :
    case 'D' :
      {
        PString cmd = "gdb " + PProcess::Current().GetFile();
        cmd.sprintf(" %d", getpid());
        system((const char *)cmd);
      }
      break;
#endif

    case 'c' :
    case 'C' :
      PError << "\nDumping core.\n";
      kill(getpid(), SIGABRT);

    case 'i' :
    case 'I' :
    case EOF :
      PError << "\nIgnoring.\n";
      return TRUE;
  }
  return FALSE;
}
#endif
#endif

void PAssertFunc(const char * msg)

{
#ifdef __BEOS__
  // Print location in Eddie-compatible format
  PError << msg << endl;
  // Pop up the debugger dialog that gives the user the necessary choices
  // "Ignore" is not supported on BeOS but you can instruct the
  // debugger to continue executing.
  // Note: if you choose "debug" you get a debug prompt. Type bdb to
  // start the Be Debugger.
  debugger(msg);
#else
  static BOOL inAssert;
  if (inAssert)
    return;
  inAssert = TRUE;

  ostream & trace = PTrace::Begin(0, __FILE__, __LINE__);
  trace << "PWLib\t" << msg << PTrace::End;

  if (&trace != &PError)
    PError << msg << endl;

  char *env;

#if P_EXCEPTIONS
  //Throw a runtime exception if the environment variable PWLIB_ASSERT_EXCEPTION is set
  env = ::getenv("PWLIB_ASSERT_EXCEPTION");
  if (env != NULL){
    throw std::runtime_error(msg);
  }
#endif
  
#ifndef P_VXWORKS
  env = ::getenv("PWLIB_ASSERT_ACTION");
  if (env != NULL && *env != EOF && PAssertAction(*env, msg)) {
    inAssert = FALSE;
    return;
  }

  // Check for if stdin is not a TTY and just ignore the assert if so.
  if (!isatty(STDIN_FILENO)) {
    inAssert = FALSE;
    return;
  }

  for(;;) {
    PError << "\n<A>bort, <C>ore dump"
#if P_EXCEPTIONS
           << ", <I>gnore <T>hrow exception"
#endif
#ifdef _DEBUG
           << ", <D>ebug"
#endif
           << "? " << flush;

    int c = getchar();

    if (PAssertAction(c, msg))
      break;
   }
   inAssert = FALSE;

#else // P_VXWORKS

  PThread::Trace(); // Get debugging dump
  exit(1);
  kill(taskIdSelf(), SIGABRT);

#endif // P_VXWORKS
#endif // __BEOS__
}

// End Of File ///////////////////////////////////////////////////////////////
