/*
**************************************************************************
*                                                                        *
*                 ____                ___ _   _____                      *
*                / __ \___  ___ ___  / _ | | / / _ \____                 *
*               / /_/ / _ \/ -_) _ \/ __ | |/ / , _/ __/                 *
*               \____/ .__/\__/_//_/_/ |_|___/_/|_|\__/                  *
*                   /_/                                                  *
*                                                                        *
*              This file is part of the OpenAVRc project.                *
*                                                                        *
*                         Based on code(s) named :                       *
*             OpenTx - https://github.com/opentx/opentx                  *
*             Deviation - https://www.deviationtx.com/                   *
*                                                                        *
*                Only AVR code here for visibility ;-)                   *
*                                                                        *
*   OpenAVRc is free software: you can redistribute it and/or modify     *
*   it under the terms of the GNU General Public License as published by *
*   the Free Software Foundation, either version 2 of the License, or    *
*   (at your option) any later version.                                  *
*                                                                        *
*   OpenAVRc is distributed in the hope that it will be useful,          *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*   GNU General Public License for more details.                         *
*                                                                        *
*       License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html          *
*                                                                        *
**************************************************************************
*/


#include "OpenAVRc_DesktopApp.h"

//(*AppHeaders
#include "OpenAVRc_DesktopMain.h"
#include <wx/image.h>
//*)

IMPLEMENT_APP(OpenAVRc_DesktopApp);

bool OpenAVRc_DesktopApp::OnInit()
{
  // Translation
  myLocale.Init(wxLANGUAGE_FRENCH , wxLOCALE_LOAD_DEFAULT);
  wxLocale::AddCatalogLookupPathPrefix(".");
  myLocale.AddCatalog("OpenAVRc_Desktop");

  //(*AppInitialize
  bool wxsOK = true;
  wxInitAllImageHandlers();
  if ( wxsOK )
  {
  	OpenAVRc_DesktopFrame* Frame = new OpenAVRc_DesktopFrame(0);
  	Frame->Show();
  	SetTopWindow(Frame);
  }
  //*)
  return wxsOK;

}
