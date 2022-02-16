#ifndef XANYCONFIGURATORFRAME_H
#define XANYCONFIGURATORFRAME_H

//(*Headers(XanyConfiguratorFrame)
#include <wx/frame.h>
//*)

class XanyConfiguratorFrame: public wxFrame
{
	public:

		XanyConfiguratorFrame(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~XanyConfiguratorFrame();

		//(*Declarations(XanyConfiguratorFrame)
		//*)

	protected:

		//(*Identifiers(XanyConfiguratorFrame)
		//*)

	private:

		//(*Handlers(XanyConfiguratorFrame)
		//*)

		DECLARE_EVENT_TABLE()
};

#endif
