// Copyright(C) 2022 by Steven Adler
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

// Oarsing APB, MWV and RMB sentences 
#include "nmea0183.h"

// Defines version numbers, names etc. for this plugin
// This is automagically constructed via version.h.in from CMakeLists.txt
#include "version.h"

// OpenCPN include file
#include "ocpn_plugin.h"

// Autopilot Dialog
#include "twocanautopilot_dialog.h"

// Some conversion calculations
// Radians to degrees and vice versa
#define RADIANS_TO_DEGREES(x) (x * 180 / M_PI)
#define DEGREES_TO_RADIANS(x) (x * M_PI / 180)

// Metres per second
#define CONVERT_MS_KNOTS 1.94384
#define CONVERT_MS_KMH 3.6
#define CONVERT_MS_MPH 2.23694

// Metres to feet, fathoms
#define CONVERT_FATHOMS_FEET 6
#define CONVERT_METRES_FEET 3.28084
#define CONVERT_METRES_FATHOMS (CONVERT_METRES_FEET / CONVERT_FATHOMS_FEET)
#define CONVERT_METRES_NAUTICAL_MILES 0.000539957

// Plugin receives events from the Autopilot Dialog
const wxEventType wxEVT_AUTOPILOT_DIALOG_EVENT = wxNewEventType();
const int AUTOPILOT_STATUS_CHANGED = wxID_HIGHEST + 1;
const int AUTOPILOT_HEADING_CHANGED = wxID_HIGHEST + 2;
const int AUTOPILOT_WAYPOINT_CHANGED = wxID_HIGHEST + 3;

// Structure to aggregate data from NMEA 183 RMB & APB Sentences and from OCPN Waypoint & Route information
// Used to generate PGN 129283 (XTE), PGN 129284 (Navigation) & PGN 129285 (Route) messages sent every second.
typedef struct _NavigationData {
	unsigned int routeId;
	std::string routeName;
	int crossTrackError; // -ve indicates to port
	bool navigationHalted;
	bool arrivalCircleEntered;
	bool perpendicularCrossed;
	unsigned int distanceToWaypoint;
	unsigned short originalBearing;
	unsigned short currentBearing;
	bool bearingReference; //True = True, False = Magnetic
	short waypointClosingVelocity;
	int destinationLatitude;
	int destinationLongitude;
	unsigned int originId;
	std::string originName;
	unsigned int destinationId;
	std::string destinationName;
} NavigationData;

// Global Variables
AUTOPILOT_MODE autopilotMode;

// The Autopilot plugin
class AutopilotPlugin : public opencpn_plugin_117, public wxEvtHandler {

public:
	// The constructor
	AutopilotPlugin(void *ppimgr);

	// and destructor
	~AutopilotPlugin(void);

protected:
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
	void SetNMEASentence(wxString &sentence);
	void SetPluginMessage(wxString &message_id, wxString &message_body);
	void SetActiveLegInfo(Plugin_Active_Leg_Info &leg_info);
	void SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix);
	void UpdateAuiStatus(void);
	void LateInit(void);

	// Event Handler for OpenCPN Plugin Messages
	void OnPluginEvent(wxCommandEvent &event);

	// AUI Manager events
	void OnPaneClose(wxAuiManagerEvent& event);

private:
	// AUI Manager
	wxAuiManager *auiManager;

	// Reference to the OpenCPN configuration file
	wxFileConfig *configSettings;

	// Reference to the OpenCPN window handle
	wxWindow *parentWindow;

	// Toolbar Id
	int autopilotToolbar;

	// Toolbar State
	bool autopilotDialogVisible;

	// Autopilot Model
	int autopilotModel;
	
	// Autopilot Dialog 
	AutopilotDialog *autopilotDialog;

	// Event Handler for events received from the Autopilot dialog
	void OnDialogEvent(wxCommandEvent &event);

	// Used to construct PGN 129285 and for UI display purposes
	PlugIn_Waypoint LookupWaypoint(wxString guid);
	wxString LookupRouteName(wxString guid);

	// Sends Keep Alive messages
	wxTimer *oneSecondTimer;
	void OnTimerElapsed(wxEvent &event);

	// For parsing NMEA 183 APB, RMB and XTE sentences
	NMEA0183 nmea183;

	// Aggregates data used to generate PGN's 129284, 129285 & 129285
	NavigationData navigationData;

};
#endif 