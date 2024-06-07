///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
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

	buttonStandby = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonStandby->SetBitmap( wxBitmap( wxT("../data/images/power-button.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonStandby, 0, wxALL, 5 );

	buttonCompass = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonCompass->SetBitmap( wxBitmap( wxT("../data/images/compass.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonCompass, 0, wxALL, 5 );

	buttonWind = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonWind->SetBitmap( wxBitmap( wxT("../data/images/wind.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonWind, 0, wxALL, 5 );

	buttonTrack = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonTrack->SetBitmap( wxBitmap( wxT("../data/images/way.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonTrack, 0, wxALL, 5 );

	buttonPortTen = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonPortTen->SetBitmap( wxBitmap( wxT("../data/images/left.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonPortTen, 0, wxALL, 5 );

	buttonStarboardTen = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonStarboardTen->SetBitmap( wxBitmap( wxT("../data/images/right.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonStarboardTen, 0, wxALL, 5 );

	buttonPortOne = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonPortOne->SetBitmap( wxBitmap( wxT("../data/images/left-arrow.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonPortOne, 0, wxALL, 5 );

	buttonStarboardOne = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	buttonStarboardOne->SetBitmap( wxBitmap( wxT("../data/images/right-arrow.png"), wxBITMAP_TYPE_ANY ) );
	sizerMode->Add( buttonStarboardOne, 0, wxALL, 5 );


	sizerFrame->Add( sizerMode, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerHeading;
	sizerHeading = new wxBoxSizer( wxVERTICAL );

	labelHeading = new wxStaticText( this, wxID_ANY, wxT("Heading"), wxDefaultPosition, wxDefaultSize, 0 );
	labelHeading->Wrap( -1 );
	sizerHeading->Add( labelHeading, 0, wxALL, 5 );

	labelNav = new wxStaticText( this, wxID_ANY, wxT("Nav Status"), wxDefaultPosition, wxDefaultSize, 0 );
	labelNav->Wrap( -1 );
	sizerHeading->Add( labelNav, 0, wxALL, 5 );

	labelStatus = new wxStaticText( this, wxID_ANY, wxT("Status"), wxDefaultPosition, wxDefaultSize, 0 );
	labelStatus->Wrap( -1 );
	sizerHeading->Add( labelStatus, 0, wxALL, 5 );

	labelAlarm = new wxStaticText( this, wxID_ANY, wxT("Alarm"), wxDefaultPosition, wxDefaultSize, 0 );
	labelAlarm->Wrap( -1 );
	sizerHeading->Add( labelAlarm, 0, wxALL, 5 );


	sizerFrame->Add( sizerHeading, 0, wxEXPAND, 5 );


	this->SetSizer( sizerFrame );
	this->Layout();

	// Connect Events
	buttonStandby->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStandby ), NULL, this );
	buttonCompass->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnCompass ), NULL, this );
	buttonWind->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnWind ), NULL, this );
	buttonTrack->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnTrack ), NULL, this );
	buttonPortTen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortTen ), NULL, this );
	buttonStarboardTen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStarboardten ), NULL, this );
	buttonPortOne->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortOne ), NULL, this );
	buttonStarboardOne->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStarboardOne ), NULL, this );
}

AutopilotDialogBase::~AutopilotDialogBase()
{
	// Disconnect Events
	buttonStandby->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStandby ), NULL, this );
	buttonCompass->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnCompass ), NULL, this );
	buttonWind->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnWind ), NULL, this );
	buttonTrack->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnTrack ), NULL, this );
	buttonPortTen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortTen ), NULL, this );
	buttonStarboardTen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStarboardten ), NULL, this );
	buttonPortOne->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnPortOne ), NULL, this );
	buttonStarboardOne->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AutopilotDialogBase::OnStarboardOne ), NULL, this );

}
