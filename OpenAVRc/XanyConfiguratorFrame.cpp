#include "XanyConfiguratorFrame.h"

//(*InternalHeaders(XanyConfiguratorFrame)
#include <wx/intl.h>
#include <wx/string.h>
//*)

//(*IdInit(XanyConfiguratorFrame)
//*)

BEGIN_EVENT_TABLE(XanyConfiguratorFrame,wxFrame)
	//(*EventTable(XanyConfiguratorFrame)
	//*)
END_EVENT_TABLE()

XanyConfiguratorFrame::XanyConfiguratorFrame(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(XanyConfiguratorFrame)
	Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("id"));
	SetClientSize(wxDefaultSize);
	Move(wxDefaultPosition);
	//*)
}

XanyConfiguratorFrame::~XanyConfiguratorFrame()
{
	//(*Destroy(XanyConfiguratorFrame)
	//*)
}

