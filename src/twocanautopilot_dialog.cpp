// Copyright(C) 2021 by Steven Adler
//
// This file is part of TwoCan Autopilot, a plugin for OpenCPN.
//
// TwoCan Autopilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TwoCan Autopilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TwoCan Autopilot. If not, see <https://www.gnu.org/licenses/>.
//
// NMEA2000® is a registered Trademark of the National Marine Electronics Association

//
// Project: TwoCan Autopilot Plugin
// Description: Rudimentary control of Autopilot Computers (via TwoCan plugin)
// Unit: Autoplot Control user dialog
// Owner: twocanplugin@hotmail.com
// Date: 01/12/2020
// Version History: 
// 1.0 Initial Release of Autopilot Control
// 1.1 - 20/02/2025 - Code cleanup, Add Rudder Angle display and Alarm labels.

#include "twocanautopilot_dialog.h"

// Constructor and destructor implementation
// inherits froms twocanautopilotsettingsbase which was implemented using wxFormBuilder
AutopilotDialog::AutopilotDialog(wxWindow* parent, wxEvtHandler *handler) : 
	AutopilotDialogBase(parent) {
		
	// Save the parent event handler address
	eventHandlerAddress = handler;
	
	Fit();
	
	// BUG BUG Should these be persisted
	autopilotMode = AUTOPILOT_MODE::STANDBY;
	EnableGPSMode(FALSE);
	EnableButtons(FALSE);
	EnableAlarm(FALSE); // Only display when there is an active alarm

	// Load bitmap buttons
	// BUG BUG How to visualize that a button is pressed/activated
	buttonWind->SetBitmap(wxBitmapBundle(*_img_wind));
	buttonNav->SetBitmap(wxBitmapBundle(*_img_track));
	buttonCompass->SetBitmap(wxBitmapBundle(*_img_compass));
	buttonStandby->SetBitmap(wxBitmapBundle(*_img_power));
	buttonPortOne->SetBitmap(wxBitmapBundle(*_img_left_one));
	buttonPortTen->SetBitmap(wxBitmapBundle(*_img_left_ten));
	buttonStarboardOne->SetBitmap(wxBitmapBundle(*_img_right_one));
	buttonStarboardTen->SetBitmap(wxBitmapBundle(*_img_right_ten));
	buttonAlarm->SetBitmap(wxBitmapBundle(*_img_alarm));

}

AutopilotDialog::~AutopilotDialog() {
	// Nothing to do
}

void AutopilotDialog::OnInit(wxActivateEvent& event) {
	// Nothing to do
}

void AutopilotDialog::OnWindowDestroy(wxWindowDestroyEvent& event) {
	if (autopilotMode != AUTOPILOT_MODE::STANDBY) {
		wxMessageBox("Please disengage autopilot before exiting",_T("Destroy"), wxICON_WARNING);
	}
	event.Skip();
}

void AutopilotDialog::OnCancel(wxCommandEvent &event) {
	// Only close if the autopilot is not powered on
	if (autopilotMode != AUTOPILOT_MODE::STANDBY) {
		wxMessageBox("Please disengage autopilot before exiting", _T("OnCancel"), wxICON_WARNING);
	}
	else {
		Close();
	}
}

void AutopilotDialog::OnClose(wxCloseEvent& event) {
	if (autopilotMode != AUTOPILOT_MODE::STANDBY) {
		wxMessageBox("Please disengage autopilot before exiting",_T("Close"), wxICON_WARNING);
		event.Veto(FALSE);
	}
	else {

		// or Destroy();
	}
}

// Events forwarded to parent are encoded as OpenCPN JSON messages and sent to TwoCan plugin 
void AutopilotDialog::RaiseEvent(int commandId, int command) {
	wxCommandEvent *event = new wxCommandEvent(wxEVT_AUTOPILOT_DIALOG_EVENT, commandId);
	event->SetInt(command);
	wxQueueEvent(eventHandlerAddress, event);
}

// Enable/Disable the course alteration buttons
void AutopilotDialog::EnableButtons(bool state) {
	buttonPortOne->Enable(state);
	buttonPortTen->Enable(state);
	buttonStarboardOne->Enable(state);
	buttonStarboardTen->Enable(state);
}

// Show/Hide Alarm Button and accompanying label
void AutopilotDialog::EnableAlarm(bool state) {
	if (!state) {
		labelAlarm->SetLabel(wxEmptyString);
		buttonAlarm->Hide();
	}
	else {
		buttonAlarm->Show();
	}
}

// BUG BUG Changing modes
// Add buttons for other modes such as Non Follow Up, No Drift, "S" curves, Depth Contour, Search Pattern
void AutopilotDialog::OnStandby(wxCommandEvent& event) {
	autopilotMode = AUTOPILOT_MODE::STANDBY;
	EnableButtons(FALSE);
	RaiseEvent(AUTOPILOT_MODE_CHANGED, autopilotMode);
}

void AutopilotDialog::OnWind(wxCommandEvent& event) {
	autopilotMode = AUTOPILOT_MODE::WIND;
	EnableButtons(TRUE);
	RaiseEvent(AUTOPILOT_MODE_CHANGED, autopilotMode);
}

void AutopilotDialog::OnCompass(wxCommandEvent& event) {
	autopilotMode = AUTOPILOT_MODE::COMPASS;
	EnableButtons(TRUE);
	RaiseEvent(AUTOPILOT_MODE_CHANGED, autopilotMode);
}

void AutopilotDialog::OnTrack(wxCommandEvent& event) {
	autopilotMode = AUTOPILOT_MODE::NAV;
	EnableButtons(TRUE);
	RaiseEvent(AUTOPILOT_MODE_CHANGED, autopilotMode);
}

void AutopilotDialog::OnSilenceAlarm(wxCommandEvent& event) {
	EnableAlarm(FALSE);
}

// Only enable the GPS mode if a route or waypoint is active
void AutopilotDialog::EnableGPSMode(bool state) {
	buttonNav->Enable(state);
}

void AutopilotDialog::OnPortTen(wxCommandEvent &event) {
	ChangeHeading(-10);
}

void AutopilotDialog::OnStbdTen(wxCommandEvent &event) {
	ChangeHeading(10);
}

void AutopilotDialog::OnPortOne(wxCommandEvent &event) {
	ChangeHeading(-1);
}

void AutopilotDialog::OnStbdOne(wxCommandEvent &event) {
	ChangeHeading(1);
}

void AutopilotDialog::ChangeHeading(int value) {
	RaiseEvent(AUTOPILOT_HEADING_CHANGED, value);
}

// Setters
void AutopilotDialog::SetMode(AUTOPILOT_MODE mode) {
	// BUG BUG Need a better way to indicate or show that the appropriate button was selected
	switch (mode) {
		case AUTOPILOT_MODE::COMPASS:
			buttonCompass->SetBackgroundColour(*wxRED);
			buttonNav->SetBackgroundColour(*wxLIGHT_GREY);
			buttonWind->SetBackgroundColour(*wxLIGHT_GREY);
			buttonStandby->SetBackgroundColour(*wxLIGHT_GREY);
			labelStatus->SetLabel("Heading");
		break;

		case AUTOPILOT_MODE::NAV:
			buttonCompass->SetBackgroundColour(*wxLIGHT_GREY);
			buttonNav->SetBackgroundColour(*wxRED);
			buttonWind->SetBackgroundColour(*wxLIGHT_GREY);
			buttonStandby->SetBackgroundColour(*wxLIGHT_GREY);;
			labelStatus->SetLabel("Navigation");
		break;

		case AUTOPILOT_MODE::WIND:
			buttonCompass->SetBackgroundColour(*wxLIGHT_GREY);
			buttonNav->SetBackgroundColour(*wxLIGHT_GREY);
			buttonWind->SetBackgroundColour(*wxRED);
			buttonStandby->SetBackgroundColour(*wxLIGHT_GREY);
			labelStatus->SetLabel("Wind");
		break;

		case AUTOPILOT_MODE::STANDBY:
			buttonCompass->SetBackgroundColour(*wxLIGHT_GREY);
			buttonNav->SetBackgroundColour(*wxLIGHT_GREY);
			buttonWind->SetBackgroundColour(*wxLIGHT_GREY);
			buttonStandby->SetBackgroundColour(*wxRED);
			labelStatus->SetLabel("Standby");
		break;

	}
}


void AutopilotDialog::SetStatusLabel(wxString statusText) {
	labelStatus->SetLabel(statusText);
}

void AutopilotDialog::SetHeadingLabel(wxString headingText) {
	labelHeading->SetLabel(headingText);
}

void AutopilotDialog::SetAlarmLabel(wxString alarmText) {
	labelAlarm->SetLabel(alarmText);
}

void AutopilotDialog::DrawRudderAngle(const int& rudderAngle) {
	wxClientDC dc(panelRudder);

	if (dc.IsOk()) {

		dc.Clear();
		// Rudder Angle display from +/- 40 degrees
		double length = (panelRudder->GetClientSize().GetWidth() / 2.0) * (rudderAngle / 40.0);

		if (rudderAngle < 0) {
			dc.SetPen(*wxRED_PEN);
			dc.SetBrush(*wxRED_BRUSH);
			dc.DrawRectangle((panelRudder->GetClientSize().GetWidth() / 2.0) + length, 0,
				abs(length), panelRudder->GetClientSize().GetHeight());
		}
		else {
			dc.SetPen(*wxGREEN_PEN);
			dc.SetBrush(*wxGREEN_BRUSH);
			dc.DrawRectangle(panelRudder->GetClientSize().GetWidth() / 2.0, 0,
				length, panelRudder->GetClientSize().GetHeight());
		}
		// Draw a scale at 10 degree intervals
		dc.SetPen(*wxBLACK_PEN);
		dc.DrawLine(0, 0, panelRudder->GetClientSize().GetWidth(), 0);
		dc.DrawLine(0, panelRudder->GetClientSize().GetHeight(), panelRudder->GetClientSize().GetWidth(),
			panelRudder->GetClientSize().GetHeight());

		double interval = panelRudder->GetClientSize().GetWidth() / 8;

		for (int i = 0; i < 8; i++) {
			dc.DrawLine(i * interval, 0, i * interval, panelRudder->GetClientSize().GetHeight());
		}
	}
}

void AutopilotDialog::OnSize(wxSizeEvent& event) {

	event.Skip();
}

