///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/bmpbuttn.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
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
		wxBitmapButton* buttonStandby;
		wxBitmapButton* buttonCompass;
		wxBitmapButton* buttonWind;
		wxBitmapButton* buttonTrack;
		wxBitmapButton* buttonPortTen;
		wxBitmapButton* buttonStarboardTen;
		wxBitmapButton* buttonPortOne;
		wxBitmapButton* buttonStarboardOne;
		wxStaticText* labelHeading;
		wxStaticText* labelNav;
		wxStaticText* labelStatus;
		wxStaticText* labelAlarm;

		// Virtual event handlers, override them in your derived class
		virtual void OnStandby( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompass( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWind( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTrack( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPortTen( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStarboardten( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPortOne( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStarboardOne( wxCommandEvent& event ) { event.Skip(); }


	public:

		AutopilotDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 178,415 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~AutopilotDialogBase();

};

