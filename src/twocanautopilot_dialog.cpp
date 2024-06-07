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
// 1.0 Initial Release of Autopilot Control - Not actually exposed or used yet....

#include "twocanautopilot_dialog.h"

// Constructor and destructor implementation
// inherits froms twocanautopilotsettingsbase which was implemented using wxFormBuilder
AutopilotDialog::AutopilotDialog(wxWindow* parent, wxEvtHandler *handler) : AutopilotDialogBase(parent) {
		
	// Save the parent event handler address
	eventHandlerAddress = handler;
	
	Fit();
	
	// BUG BUG Should these be persisted
	autopilotMode = AUTOPILOT_MODE::STANDBY;
	EnableGPSMode(FALSE);
	EnableButtons(FALSE);
	
}

AutopilotDialog::~AutopilotDialog() {
// Nothing to do
}

void AutopilotDialog::OnInit(wxActivateEvent& event) {
// Nothing to do
}

void AutopilotDialog::OnWindowDestroy(wxWindowDestroyEvent& event) {
	if (autopilotMode != _AUTOPILOT_MODE::STANDBY) {
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

void AutopilotDialog::RaiseEvent(int commandId, int command) {
	wxCommandEvent *event = new wxCommandEvent(wxEVT_AUTOPILOT_DIALOG_EVENT, commandId);
	event->SetInt(command);
	wxQueueEvent(eventHandlerAddress, event);
}


void AutopilotDialog::EnableButtons(bool state) {
	// Enable/Disable the course alteration buttons
	buttonPortOne->Enable(state);
	buttonPortTen->Enable(state);
	buttonStarboardOne->Enable(state);
	buttonStarboardTen->Enable(state);
}

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

// Only enable the GPS mode if a route or waypoint is active
void AutopilotDialog::EnableGPSMode(bool state) {
	buttonTrack->Enable(state);
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
	// Need to indicate or show that the approprite button was selected
	switch (mode) {
	case AUTOPILOT_MODE::COMPASS:
		buttonCompass->SetBackgroundColour(*wxBLUE);
		break;

	case AUTOPILOT_MODE::NAV:
		buttonTrack->SetBackgroundColour(*wxGREEN);
		break;

	case AUTOPILOT_MODE::WIND:
		buttonWind->SetBackgroundColour(*wxLIGHT_GREY);
		break;

	case AUTOPILOT_MODE::STANDBY:
		buttonStandby->SetBackgroundColour(*wxRED);

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