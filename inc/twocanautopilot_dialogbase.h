///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class AutopilotDialogBase
///////////////////////////////////////////////////////////////////////////////
class AutopilotDialogBase : public wxPanel
{
	private:

	protected:
		wxRadioBox* radioBoxStatus;
		wxButton* buttonPortTen;
		wxButton* buttonStbdTen;
		wxButton* buttonPortOne;
		wxButton* buttonStbdOne;
		wxStaticText* labelHeading;
		wxStaticText* labelStatus;
		wxStaticText* labelAlarm;

		// Virtual event handlers, overide them in your derived class
		virtual void OnStatusChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPortTen( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStbdTen( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPortOne( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStbdOne( wxCommandEvent& event ) { event.Skip(); }


	public:

		AutopilotDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 248,415 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~AutopilotDialogBase();

};

