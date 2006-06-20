/*
 * main.cxx
 *
 * PWLib application source file for emailtest
 *
 * Main program entry point.
 *
 * Copyright (c) 2004 Post Increment
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
 * $Log: main.cxx,v $
 * Revision 1.2  2006/06/20 09:23:56  csoutheren
 * Applied patch 1465192
 * Fix pwlib make files, and config for unix
 *
 * Revision 1.1  2004/08/11 07:39:05  csoutheren
 * Initial version
 *
 */

#include "precompile.h"
#include "main.h"
#include "version.h"

#include <ptlib/sockets.h>
#include <ptclib/inetmail.h>

PCREATE_PROCESS(Emailtest);

Emailtest::Emailtest()
  : PProcess("Post Increment", "emailtest", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

void Emailtest::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "h.   "
             "v.    "
             "h.   "
             "v.    "
             "h.   "
             "v.    "
             "-server:"
             "-to:"
             "-from:"
             "-re:"
             "-attachment:"

#if PTRACING
             "o-output:"             "-no-output."
 
  if (args.HasOption('h')) {
    cout << "usage: " <<  (const char *)GetName()
         << endl
         << "     -server        : mail server" << endl
         << "     -to      : to address" << endl
         << "     -from   : from address " << endl
         << "     -re   : subject " << endl
         << "     -attachment   : list of attachments " << endl
         << "     -h        : get help on usage " << endl
         << "     -v        : report program version " << endl
#if PTRACING
         << "  -t --trace   : Enable trace, use multiple times for more detail" << endl
         << "  -o --output  : File for trace output, default is stderr" << endl
#endif
         << endl;
    return;
  }




             "t-trace."              "-no-trace."
#endif
 
  if (args.HasOption('h')) {
    cout << "usage: " <<  (const char *)GetName()
         << endl
 if (args.HasOption('v')) {
    cout << endl
         << "Product Name: " <<  (const char *)GetName() << endl
         << "Manufacturer: " <<  (const char *)GetManufacturer() << endl
         << "Version     : " <<  (const char *)GetVersion(TRUE) << endl
         << "System      : " <<  (const char *)GetOSName() << '-'
         <<  (const char *)GetOSHardware() << ' '
         <<  (const char *)GetOSVersion() << endl
         << endl;
    return;
  }
  
         << "     -server        : mail server" << endl
         << "     -to      : to address" << endl
         << "     -from   : from address " << endl
         << "     -re   : subject " << endl
         << "     -attachment   : list of attachments " << endl
         << "     -h        : get help on usage " << endl
         << "     -v        : report program version " << endl
#if PTRACING
         << "  -t --trace   : Enable trace, use multiple times for more detail" << endl
         << "  -o --output  : File for trace output, default is stderr" << endl
#endif
         << endl;
    return;
  }




  );

 
  if (args.HasOption('h')) {
    cout << "usage: " <<  (const char *)GetName()
         << endl
 if (args.HasOption('v')) {
    cout << endl
         << "Product Name: " <<  (const char *)GetName() << endl
         << "Manufacturer: " <<  (const char *)GetManufacturer() << endl
         << "Version     : " <<  (const char *)GetVersion(TRUE) << endl
         << "System      : " <<  (const char *)GetOSName() << '-'
         <<  (const char *)GetOSHardware() << ' '
         <<  (const char *)GetOSVersion() << endl
         << endl;
    return;
  }
  
         << "     -server        : mail server" << endl
         << "     -to      : to address" << endl
         << "     -from   : from address " << endl
         << "     -re   : subject " << endl
         << "     -attachment   : list of attachments " << endl
         << "     -h        : get help on usage " << endl
         << "     -v        : report program version " << endl
#if PTRACING
         << "  -t --trace   : Enable trace, use multiple times for more detail" << endl
         << "  -o --output  : File for trace output, default is stderr" << endl
#endif
         << endl;
    return;
  }




#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

 if (args.HasOption('v')) {
    cout << endl
         << "Product Name: " <<  (const char *)GetName() << endl
         << "Manufacturer: " <<  (const char *)GetManufacturer() << endl
         << "Version     : " <<  (const char *)GetVersion(TRUE) << endl
         << "System      : " <<  (const char *)GetOSName() << '-'
         <<  (const char *)GetOSHardware() << ' '
         <<  (const char *)GetOSVersion() << endl
         << endl;
    return;
  }
  
  PRFC822Channel email(PRFC822Channel::Sending);

  PString to = args.GetOptionString("to");
  PString from = args.GetOptionString("from");

  email.SetToAddress(to);
  email.SetFromAddress(from);
  email.SetSubject(args.GetOptionString("re"));

  PStringArray attachments = args.GetOptionString("attachment").Lines();

  PString server = args.GetOptionString("server");
  if (server.IsEmpty())
    server = "127.0.0.1";

  PTCPSocket socket("smtp 25");
  if (!socket.Connect(server)) {
    PError << "error: could not connect to SMTP server " << server << endl;
    return;
  }

  PSMTPClient smtpClient;
  if (!smtpClient.Open(socket)) {
    PError << "error: could not open SMTP server " << server << endl;
    return;
  }

  if (!email.Open(smtpClient)) {
    PError << "error: cannot open email message " << server << endl;
    return;
  }

  if (!smtpClient.BeginMessage(from, to)) {
    PError << "error: could not begin SMTP message " << smtpClient.GetErrorText() << endl;
    return;
  }

  PString boundary;
  if (attachments.GetSize() > 0) {
    boundary = email.MultipartMessage();
  }

  for (PINDEX i = 0; i < args.GetCount(); ++i) {
    email.Write((const char *)args[i], args[i].GetLength());
    email << "\n";
  }

  if (attachments.GetSize() > 0) {
    for (PINDEX i = 0; i < attachments.GetSize(); ++i) {
      PFilePath filename = attachments[i];
      PFile file(filename, PFile::ReadOnly);
      if (file.IsOpen()) {
        email.NextPart(boundary);
        email.SetContentAttachment(filename.GetFileName());
        PString fileType = filename.GetType();
        PString contentType = PMIMEInfo::GetContentType(fileType);
        if ((fileType *= "txt") || (fileType == "html"))
          email.SetTransferEncoding("7bit", FALSE);
        else
          email.SetTransferEncoding("base64", TRUE);
        BYTE buffer[1024];
        for (;;) {
          if (!file.Read(buffer, sizeof(buffer)))
            break;
          email.Write(buffer, file.GetLastReadCount());
        }
      }
    }
  }

  smtpClient.EndMessage();

  email.Close();
}


// End of File ///////////////////////////////////////////////////////////////
