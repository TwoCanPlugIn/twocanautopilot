///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "twocanautopilot_dialogbase.h"

///////////////////////////////////////////////////////////////////////////

AutopilotDialogBase::AutopilotDialogBase( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* sizerFrame;
	sizerFrame = new wxBoxSizer( wxVERTICAL );

	wxGridSizer* sizerMode;
	sizerMode = new wxGridSizer( 4, 2, 0, 0 );

	buttonStandby = new wxBitmapToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sizerMode->Add( buttonStandby, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttonCompass = new wxBitmapToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sizerMode->Add( buttonCompass, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttonWind = new wxBitmapToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sizerMode->Add( buttonWind, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttonNav = new wxBitmapToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sizerMode->Add( buttonNav, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttonPortTen = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	sizerMode->Add( buttonPortTen, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttonStarboardTen = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	sizerMode->Add( buttonStarboardTen, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	buttonPortOne = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	sizerMode->Add( buttonPortOne, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttonStarboardOne = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	sizerMode->Add( buttonStarboardOne, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sizerFrame->Add( sizerMode, 5, wxEXPAND, 5 );

	wxBoxSizer* sizerHeading;
	sizerHeading = new wxBoxSizer( wxVERTICAL );

	labelStatus = new wxStaticText( this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	labelStatus->Wrap( -1 );
	sizerHeading->Add( labelStatus, 1, wxALL, 5 );

	labelHeading = new wxStaticText( this, wxID_ANY, wxT("Heading"), wxDefaultPosition, wxDefaultSize, 0 );
	labelHeading->Wrap( -1 );
	sizerHeading->Add( labelHeading, 1, wxALL, 5 );

	labelAlarm = new wxStaticText( this, wxID_ANY, wxT("Alarm"), wxDefaultPosition, wxDefaultSize, 0 );
	labelAlarm->Wrap( -1 );
	sizerHeading->Add( labelAlarm, 1, wxALL, 5 );

	buttonAlarm = new wxBitmapToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sizerHeading->Add( buttonAlarm, 0, wxALL, 5 );


	sizerFrame->Add( sizerHeading, 3, wxEXPAND, 5 );

	wxBoxSizer* sizerRudder;
	sizerRudder = new wxBoxSizer( wxHORIZONTAL );

	panelRudder = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	sizerRudder->Add( panelRudder, 1, wxALL|wxEXPAND, 5 );


	sizerFrame->Add( sizerRudder, 1, wxEXPAND, 5 );


	this->SetSizer( sizerFrame );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( AutopilotDialogBase::OnInitDialog ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( AutopilotDialogBase::OnSize ) );
	buttonStandby->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStandby ), NULL, this );
	buttonCompass->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnCompass ), NULL, this );
	buttonWind->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnWind ), NULL, this );
	buttonNav->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnNav ), NULL, this );
	buttonPortTen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortTen ), NULL, this );
	buttonStarboardTen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdTen ), NULL, this );
	buttonPortOne->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortOne ), NULL, this );
	buttonStarboardOne->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdOne ), NULL, this );
	buttonAlarm->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnAlarm ), NULL, this );
	panelRudder->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( AutopilotDialogBase::OnEraseBackground ), NULL, this );
}

AutopilotDialogBase::~AutopilotDialogBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( AutopilotDialogBase::OnInitDialog ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( AutopilotDialogBase::OnSize ) );
	buttonStandby->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStandby ), NULL, this );
	buttonCompass->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnCompass ), NULL, this );
	buttonWind->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnWind ), NULL, this );
	buttonNav->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnNav ), NULL, this );
	buttonPortTen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortTen ), NULL, this );
	buttonStarboardTen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdTen ), NULL, this );
	buttonPortOne->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortOne ), NULL, this );
	buttonStarboardOne->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdOne ), NULL, this );
	buttonAlarm->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnAlarm ), NULL, this );
	panelRudder->Disconnect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( AutopilotDialogBase::OnEraseBackground ), NULL, this );

}
