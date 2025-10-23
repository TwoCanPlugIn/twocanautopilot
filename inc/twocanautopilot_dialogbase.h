///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/tglbtn.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bmpbuttn.h>
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
		wxBitmapToggleButton* buttonStandby;
		wxBitmapToggleButton* buttonCompass;
		wxBitmapToggleButton* buttonWind;
		wxBitmapToggleButton* buttonNav;
		wxBitmapButton* buttonPortTen;
		wxBitmapButton* buttonStarboardTen;
		wxBitmapButton* buttonPortOne;
		wxBitmapButton* buttonStarboardOne;
		wxStaticText* labelStatus;
		wxStaticText* labelHeading;
		wxStaticText* labelAlarm;
		wxBitmapToggleButton* buttonAlarm;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnStandby( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompass( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWind( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNav( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPortTen( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStbdTen( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPortOne( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStbdOne( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAlarm( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEraseBackground( wxEraseEvent& event ) { event.Skip(); }


	public:
		wxPanel* panelRudder;

		AutopilotDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 238,460 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~AutopilotDialogBase();

};

