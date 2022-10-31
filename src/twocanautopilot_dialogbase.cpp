///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
	sizerMode = new wxGridSizer( 0, 2, 0, 0 );

	wxString radioBoxStatusChoices[] = { wxT("Standby"), wxT("Heading"), wxT("GPS"), wxT("Wind") };
	int radioBoxStatusNChoices = sizeof( radioBoxStatusChoices ) / sizeof( wxString );
	radioBoxStatus = new wxRadioBox( this, wxID_ANY, wxT("Mode"), wxDefaultPosition, wxDefaultSize, radioBoxStatusNChoices, radioBoxStatusChoices, 1, wxRA_SPECIFY_COLS );
	radioBoxStatus->SetSelection( 1 );
	sizerMode->Add( radioBoxStatus, 0, wxALL, 5 );


	sizerFrame->Add( sizerMode, 0, 0, 5 );

	wxGridSizer* sizerLeftRight;
	sizerLeftRight = new wxGridSizer( 2, 2, 0, 0 );

	buttonPortTen = new wxButton( this, wxID_ANY, wxT("<< 10"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerLeftRight->Add( buttonPortTen, 0, wxALL, 5 );

	buttonStbdTen = new wxButton( this, wxID_ANY, wxT("10 >>"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerLeftRight->Add( buttonStbdTen, 0, wxALL, 5 );

	buttonPortOne = new wxButton( this, wxID_ANY, wxT("<< 1"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerLeftRight->Add( buttonPortOne, 0, wxALL, 5 );

	buttonStbdOne = new wxButton( this, wxID_ANY, wxT("1 >>"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerLeftRight->Add( buttonStbdOne, 0, wxALL, 5 );


	sizerFrame->Add( sizerLeftRight, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerHeading;
	sizerHeading = new wxBoxSizer( wxVERTICAL );

	labelHeading = new wxStaticText( this, wxID_ANY, wxT("Heading"), wxDefaultPosition, wxDefaultSize, 0 );
	labelHeading->Wrap( -1 );
	sizerHeading->Add( labelHeading, 0, wxALL, 5 );


	sizerFrame->Add( sizerHeading, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerStatus;
	sizerStatus = new wxBoxSizer( wxVERTICAL );

	labelStatus = new wxStaticText( this, wxID_ANY, wxT("Status"), wxDefaultPosition, wxDefaultSize, 0 );
	labelStatus->Wrap( -1 );
	sizerStatus->Add( labelStatus, 0, wxALL, 5 );


	sizerFrame->Add( sizerStatus, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerAlarm;
	sizerAlarm = new wxBoxSizer( wxVERTICAL );

	labelAlarm = new wxStaticText( this, wxID_ANY, wxT("Alarm"), wxDefaultPosition, wxDefaultSize, 0 );
	labelAlarm->Wrap( -1 );
	sizerAlarm->Add( labelAlarm, 0, wxALL, 5 );


	sizerFrame->Add( sizerAlarm, 0, wxEXPAND, 5 );


	this->SetSizer( sizerFrame );
	this->Layout();

	// Connect Events
	radioBoxStatus->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( AutopilotDialogBase::OnStatusChanged ), NULL, this );
	buttonPortTen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortTen ), NULL, this );
	buttonStbdTen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdTen ), NULL, this );
	buttonPortOne->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortOne ), NULL, this );
	buttonStbdOne->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdOne ), NULL, this );
}

AutopilotDialogBase::~AutopilotDialogBase()
{
	// Disconnect Events
	radioBoxStatus->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( AutopilotDialogBase::OnStatusChanged ), NULL, this );
	buttonPortTen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortTen ), NULL, this );
	buttonStbdTen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdTen ), NULL, this );
	buttonPortOne->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortOne ), NULL, this );
	buttonStbdOne->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStbdOne ), NULL, this );

}
