// Copyright(C) 2018-2020 by Steven Adler
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
// Description: Rudimentary control of Autopilot Computers (via TwoCan plugin)N
// Unit: Autoplot Control user dialog
// Owner: twocanplugin@hotmail.com
// Date: 01/12/2020
// Version History: 
// 1.0 Initial Release of Autopilot Control - Not actually exposed or used yet....



#include "twocanautopilot_dialog.h"

// Constructor and destructor implementation
// inherits froms twocanautopilotsettingsbase which was implemented using wxFormBuilder
AutopilotDialog::AutopilotDialog( wxWindow* parent, wxEvtHandler *handler) 
	: AutopilotDialogBase(parent, -1, _T("TwoCan Autopilot"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE) {
		
	// Save the parent event handler address
	eventHandlerAddress = handler;
	
	Fit();
	
	autopilotStatus = 0;
	autopilotMode = 0;
	desiredHeading = 0;
	radioBoxPower->SetSelection(autopilotStatus);
	radioBoxMode->SetSelection(autopilotMode);
	labelHeading->SetLabel(wxString::Format("%d",desiredHeading));
	
}


AutopilotDialog::~AutopilotDialog() {
// Nothing to do
}

void AutopilotDialog::OnInit(wxActivateEvent& event) {
	event.Skip();
}

void AutopilotDialog::OnWindowDestroy(wxWindowDestroyEvent& event) {
	if (autopilotStatus > 0) {
		wxMessageBox("Please disengage autopilot before exiting",_T("Destroy"), wxICON_WARNING);
		//event.Skip();
	}
}


void AutopilotDialog::OnClose(wxCloseEvent& event) {
	if (autopilotStatus > 0) {
		wxMessageBox("Please disengage autopilot before exiting",_T("Close"), wxICON_WARNING);
		event.Veto(false);
	}
	else {
		event.Skip();
		// or Destroy();
	}
}

void AutopilotDialog::RaiseEvent(int commandId, int command) {
	wxCommandEvent *event = new wxCommandEvent(wxEVT_AUTOPILOT_DIALOG_EVENT, commandId);
	event->SetString(std::to_string(command));
	wxQueueEvent(eventHandlerAddress, event);
}


void AutopilotDialog::OnPowerChanged(wxCommandEvent &event) {
	autopilotStatus = radioBoxPower->GetSelection();
	RaiseEvent(AUTOPILOT_ON, autopilotStatus);
}


void AutopilotDialog::OnModeChanged(wxCommandEvent &event) {
	autopilotMode = radioBoxMode->GetSelection();
	RaiseEvent(AUTOPILOT_MODE_HEADING, autopilotMode);
}


void AutopilotDialog::OnPortTen(wxCommandEvent &event) {
	desiredHeading -= 10;
	labelHeading->SetLabel(wxString::Format("%d",desiredHeading));
	RaiseEvent(AUTOPILOT_HEADING_MINUS_TEN, -10);
}


void AutopilotDialog::OnStbdTen(wxCommandEvent &event) {
	desiredHeading += 10;
	labelHeading->SetLabel(wxString::Format("%d",desiredHeading));
	RaiseEvent(AUTOPILOT_HEADING_PLUS_TEN, 10);
}


void AutopilotDialog::OnPortOne(wxCommandEvent &event) {
	desiredHeading -= 1;
	labelHeading->SetLabel(wxString::Format("%d",desiredHeading));
	RaiseEvent(AUTOPILOT_HEADING_MINUS_ONE, -1);
}


void AutopilotDialog::OnStbdOne(wxCommandEvent &event) {
	desiredHeading += 1;
	labelHeading->SetLabel(wxString::Format("%d",desiredHeading));
	RaiseEvent(AUTOPILOT_HEADING_PLUS_ONE, 1);
}


void AutopilotDialog::OnCancel(wxCommandEvent &event) {
// Only close if the autopilot is not powered on
	if (autopilotStatus > 0) {
		wxMessageBox("Please disengage autopilot before exiting",_T("OnCancel"), wxICON_WARNING);
		event.Skip(false);
	}
	else {
		//Close();
		EndModal(wxID_OK);
	}
}

