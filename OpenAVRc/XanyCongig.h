#ifndef XANYCONGIG_H
#define XANYCONGIG_H

//(*Headers(XanyCongig)
#include <wx/panel.h>
//*)

class XanyCongig: public wxPanel
{
	public:

		XanyCongig(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~XanyCongig();

		//(*Declarations(XanyCongig)
		//*)

	protected:

		//(*Identifiers(XanyCongig)
		//*)

	private:

		//(*Handlers(XanyCongig)
		//*)

		DECLARE_EVENT_TABLE()
};

#endif
