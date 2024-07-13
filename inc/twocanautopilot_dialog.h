// Copyright(C) 2018-2020 by Steven Adler
//
// This file is part of TwoCan Autopilot plugin for OpenCPN.
//
// TwoCan Autopilot plugin for OpenCPN is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TwoCan Autopilot plugin for OpenCPN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the TwoCan Autopilot plugin for OpenCPN. If not, see <https://www.gnu.org/licenses/>.


#ifndef TWOCAN_AUTOPILOT_DIALOG_H
#define TWOCAN_AUTOPILOT_DIALOG_H

// The settings dialog base class from which we are derived
// Note wxFormBuilder used to generate UI
#include "twocanautopilot_dialogbase.h"

#include "twocanautopilot_images.h"

// For logging
#include <wx/log.h>
#include <wx/msgdlg.h>

// Events passed up to the plugin
extern const wxEventType wxEVT_AUTOPILOT_DIALOG_EVENT;
extern const int AUTOPILOT_MODE_CHANGED;
extern const int AUTOPILOT_HEADING_CHANGED;

// Different autopilot modes
typedef enum _AUTOPILOT_MODE {
	STANDBY,
	COMPASS,
	NAV,
	WIND,
	NODRIFT,
	NFU,
	STURN,
	UTURN
} AUTOPILOT_MODE;

// Global vble indicating autopilot mode of operation
extern AUTOPILOT_MODE autopilotMode;

class AutopilotDialog : public AutopilotDialogBase {
	
public:
	AutopilotDialog(wxWindow *parent, wxEvtHandler *handler);
	~AutopilotDialog();

	// Pointer to event handler address to handle dialog requests, ie. the TwoCan Autopilot Plugin
	wxEvtHandler *eventHandlerAddress;

	// Event raised when an autopilot command is issued from the dialog
	void RaiseEvent(int commandId, int command);

	// Setters for the dialog
	void SetStatusLabel(wxString statusText);
	void SetHeadingLabel(wxString headingText);
	void SetAlarmLabel(wxString alarmText);
	// Only enable GPS mode when a route or waypoint is active
	void EnableGPSMode(bool state);
	void SetMode(AUTOPILOT_MODE mode);
	
	
protected:
	//overridden methods from the base class
	void OnInit(wxActivateEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnWindowDestroy(wxWindowDestroyEvent& event);
	void OnPortTen(wxCommandEvent &event);
	void OnStbdTen(wxCommandEvent &event);
	void OnPortOne(wxCommandEvent &event);
	void OnStbdOne(wxCommandEvent &event);
	void OnStandby(wxCommandEvent& event);
	void OnCompass(wxCommandEvent& event);
	void OnWind(wxCommandEvent& event);
	void OnTrack(wxCommandEvent& event);


private:
	void ChangeHeading(int value);
	void EnableButtons(bool enable);
};

#endif
