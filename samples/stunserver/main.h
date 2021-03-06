/*
 * main.h
 *
 * PWLib application header file for stunserver
 *
 * Copyright (c) 2010 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _StunServer_MAIN_H
#define _StunServer_MAIN_H

#include <ptlib/pprocess.h>

class StunServer : public PProcess
{
  PCLASSINFO(StunServer, PProcess)

  public:
    StunServer();
    virtual void Main();
};


#endif  // _StunServer_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
