// Copyright(C) 2018-2020 by Steven Adler
//
// This file is part of Actisense plugin for OpenCPN.
//


#ifndef TWOCAN_AUTOPILOT_DIALOG_H
#define TWOCAN_AUTOPILOT_DIALOG_H

// The settings dialog base class from which we are derived
// Note wxFormBuilder used to generate UI
#include "twocanautopilot_dialogbase.h"

// For logging
#include <wx/log.h>
#include <wx/msgdlg.h>

// Events passed up to the plugin
extern const wxEventType wxEVT_AUTOPILOT_DIALOG_EVENT;
extern const int AUTOPILOT_ON;
extern const int AUTOPILOT_STANDBY;
extern const int AUTOPILOT_OFF;
extern const int AUTOPILOT_MODE_HEADING;
extern const int AUTOPILOT_MODE_WIND;
extern const int AUTOPILOT_MODE_NAV;
extern const int AUTOPILOT_TACK_PORT;
extern const int AUTOPILOT_TACK_STBD;
extern const int AUTOPILOT_HEADING_PLUS_ONE;
extern const int AUTOPILOT_HEADING_MINUS_ONE;
extern const int AUTOPILOT_HEADING_PLUS_TEN;
extern const int AUTOPILOT_HEADING_MINUS_TEN;
extern const int AUTOPILOT_MODEL;

class AutopilotDialog : public AutopilotDialogBase {
	
public:
	AutopilotDialog(wxWindow *parent, wxEvtHandler *handler);
	~AutopilotDialog();

	// Reference to event handler address, ie. the TwoCan PlugIn
	wxEvtHandler *eventHandlerAddress;

	// Event raised when an autopilot command is issued
	void RaiseEvent(int commandId, int command);
	
protected:
	//overridden methods from the base class
	void OnInit(wxActivateEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnWindowDestroy(wxWindowDestroyEvent& event);
	void OnModeChanged(wxCommandEvent &event);
	void OnPowerChanged(wxCommandEvent &event);
	void OnPortTen(wxCommandEvent &event);
	void OnStbdTen(wxCommandEvent &event);
	void OnPortOne(wxCommandEvent &event);
	void OnStbdOne(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	
private:
	// BUG BUG perhaps use an enum
	int autopilotMode;
	int autopilotStatus;
	int desiredHeading;
		
};

#endif
