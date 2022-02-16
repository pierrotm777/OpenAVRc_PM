#include "XanyCongig.h"

//(*InternalHeaders(XanyCongig)
#include <wx/intl.h>
#include <wx/string.h>
//*)

//(*IdInit(XanyCongig)
//*)

BEGIN_EVENT_TABLE(XanyCongig,wxPanel)
	//(*EventTable(XanyCongig)
	//*)
END_EVENT_TABLE()

XanyCongig::XanyCongig(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(XanyCongig)
	Create(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("id"));
	//*)
}

XanyCongig::~XanyCongig()
{
	//(*Destroy(XanyCongig)
	//*)
}

