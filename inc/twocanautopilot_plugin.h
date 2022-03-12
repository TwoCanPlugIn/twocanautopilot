// Copyright(C) 2021 by Steven Adler
//
// This file is part of TwoCan Autopilot plugin for OpenCPN.
//
// TwoCan Autopilot plugin for OpenCPN is free software: you can 
// redistribute it and/or modify  it under the terms of the GNU General 
// Public License as published by the Free Software Foundation, either version 3 
// of the License, or (at your option) any later version.
//
// TwoCan Autopilot plugin for OpenCPN is distributed in the hope that 
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the TwoCan Autopilot plugin for OpenCPN. 
// If not, see <https://www.gnu.org/licenses/>.
//
// NMEA2000® is a registered trademark of the National Marine Electronics Association


#ifndef AUTOPILOT_PLUGIN_H
#define AUTOPILOT_PLUGIN_H

// Pre compiled headers 
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
      #include <wx/wx.h>
#endif

// wxWidgets requirements
// Advanced User Interface
#include "wx/aui/aui.h"
#include <wx/aui/framemanager.h>

// Configuration
#include <wx/fileconf.h>

// JSON
#include "json_defs.h"
#include "jsonval.h"
#include "jsonreader.h"
#include "jsonwriter.h"

// STL
#include <unordered_map>

// Defines version numbers, names etc. for this plugin
// This is automagically constructed via version.h.in from CMakeLists.txt
#include "version.h"

// OpenCPN include file
#include "ocpn_plugin.h"

// Autopilot Dialog
#include "twocanautopilot_dialog.h"

// Plugin receives events from the Autopilot Dialog
const wxEventType wxEVT_AUTOPILOT_DIALOG_EVENT = wxNewEventType();
const int AUTOPILOT_ON = wxID_HIGHEST + 1;
const int AUTOPILOT_STANDBY = wxID_HIGHEST + 2;
const int AUTOPILOT_OFF = wxID_HIGHEST + 3;
const int AUTOPILOT_MODE_HEADING = wxID_HIGHEST + 4;
const int AUTOPILOT_MODE_WIND = wxID_HIGHEST + 5;
const int AUTOPILOT_MODE_NAV = wxID_HIGHEST + 6;
const int AUTOPILOT_TACK_PORT = wxID_HIGHEST + 7;
const int AUTOPILOT_TACK_STBD = wxID_HIGHEST + 8;
const int AUTOPILOT_HEADING_PLUS_ONE = wxID_HIGHEST + 9;
const int AUTOPILOT_HEADING_MINUS_ONE = wxID_HIGHEST + 10;
const int AUTOPILOT_HEADING_PLUS_TEN = wxID_HIGHEST + 11;
const int AUTOPILOT_HEADING_MINUS_TEN = wxID_HIGHEST + 12;
const int AUTOPILOT_MODEL = wxID_HIGHEST + 13;


// The Autopilot plugin
class AutopilotPlugin : public opencpn_plugin_117, public wxEvtHandler {

public:
	// The constructor
	AutopilotPlugin(void *ppimgr);

	// and destructor
	~AutopilotPlugin(void);

	// Overridden OpenCPN plugin methods
	int Init(void);
	bool DeInit(void);
	int GetAPIVersionMajor();
	int GetAPIVersionMinor();
	int GetPlugInVersionMajor();
	int GetPlugInVersionMinor();
	wxString GetCommonName();
	wxString GetShortDescription();
	wxString GetLongDescription();
	wxBitmap *GetPlugInBitmap();
	int GetToolbarToolCount(void);
	int GetToolbarItemId(void);
	void OnToolbarToolCallback(int id);
	void SetDefaults(void);
	void SetPluginMessage(wxString &message_id, wxString &message_body);
	void UpdateAuiStatus(void);
	void LateInit(void);

	// Event Handler for OpenCPN Plugin Messages
	void OnPluginEvent(wxCommandEvent &event);

	// Event Handler for events received from the Autopilot dialog
	void OnDialogEvent(wxCommandEvent &event);

	// AUI Manager events
	void OnPaneClose(wxAuiManagerEvent& event);

private:
	// AUI Manager
	wxAuiManager *auiManager;

	// Reference to the OpenCPN configuration file
	wxFileConfig *configSettings;

	// Autopilot Dialog 
	AutopilotDialog *autopilotDialog;

	// Plugin bitmap
	wxBitmap pluginBitmap;

	// Reference to the OpenCPN window handle
	wxWindow *parentWindow;

	// Toolbar Id
	int autopilotToolbar;

	// Toolbar State
	bool autopilotDialogVisible;

	// Autopilot Model
	wxString autopilotModel;

};
#endif 