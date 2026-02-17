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

// STL
#include <vector>
#include <string>
#include <cmath>

// Defines version numbers, names etc. for this plugin
// This is automagically constructed via version.h.in from CMakeLists.txt, a bit convoluted...
#include "version.h"

// OpenCPN Plugin header
#include "ocpn_plugin.h"

// NMEA 0183, Refer to OpenCPN Libraries
#include "nmea0183.h"

// NMEA 2000, Refer to OpenCPN Libraries
#include "N2KParser.h"

// wxJSON, Refer to OpenCPN Libraries
// Used for parsing SignalK data
#include "wx/json_defs.h"
#include "wx/jsonreader.h"
#include "wx/jsonval.h"
#include "wx/jsonwriter.h"

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

std::vector<std::string>statusLabels = { "Standby", "Heading", "Track", "Wind", "No Drift",
"Non Follow Up", "S-Turn", "U-Turn" };

// Plugin receives events from the Autopilot Dialog
const wxEventType wxEVT_AUTOPILOT_DIALOG_EVENT = wxNewEventType();
const int AUTOPILOT_MODE_CHANGED = wxID_HIGHEST + 1;
const int AUTOPILOT_HEADING_CHANGED = wxID_HIGHEST + 2;

// Structure to aggregate data from NMEA 183 RMB & APB Sentences and from OCPN Waypoint & Route information
// Used to generate PGN 129283 (XTE), PGN 129284 (Navigation) & PGN 129285 (Route) messages sent every second.
// All data stored in Imperial Units (Nautical Miles, Knots, Degrees etc.)
// BUG BUG, Should check the user data units for data retrieved from OpenCPN functions
// Perform conversion to SI units for NMEA 2000 in the sending routines
typedef struct _NavigationData {
	unsigned int routeId;
	std::string routeName;
	double crossTrackError; // -ve indicates to port
	int xteMode;
	bool navigationHalted;
	bool arrivalCircleEntered;
	bool perpendicularCrossed;
	double distanceToWaypoint;
	double originalBearing;
	double currentBearing;
	bool bearingReference; //True = TRUE, False = Magnetic
	double waypointClosingVelocity;
	double destinationLatitude;
	double destinationLongitude;
	unsigned int originId;
	std::string originName;
	unsigned int destinationId;
	std::string destinationName;

	// Calculate ETA (Note assumes distance in Nm and speed in knots)
	void GetETA(unsigned short *days, unsigned int *seconds) {
		if (waypointClosingVelocity > 0) {
			wxDateTime now = wxDateTime::Now();
			wxDateTime epoch((time_t)0);
			double elapsedTime = distanceToWaypoint / waypointClosingVelocity;
			unsigned int hours = floor(elapsedTime);
			unsigned int minutes = round((elapsedTime - hours) * 60);
			now.Add(wxTimeSpan::Hours(hours));
			now.Add(wxTimeSpan::Minutes(minutes));
			wxTimeSpan dateDiff = now - epoch;
			*days = dateDiff.GetDays();
			*seconds = ((dateDiff.GetSeconds() - (*days * 86400)).GetValue()) * 10000;
		}
		else {
			*days = USHRT_MAX;
			*seconds = UINT_MAX;
		}
	}
} NavigationData;

// Global Variables
// Autopilot mode of operation, standby, auto (aka heading or compass), navigation (aka track), wind
AUTOPILOT_MODE autopilotMode;

// When the user selects heading, nav or wind mode, persist the set value.
// This is displayed in the dialog to differentiate actual vs set values; 
double desiredHeading;
double desiredWindAngle;
double waypointHeading;
double currentHeading;

// The Autopilot plugin
class AutopilotPlugin : public opencpn_plugin_120, public wxEvtHandler {

public:
	// The constructor
	AutopilotPlugin(void *ppimgr);

	// and destructor
	~AutopilotPlugin(void);

protected:
	// Overridden OpenCPN plugin methods
	int Init(void) override;
	bool DeInit(void) override;
	int GetAPIVersionMajor() override;
	int GetAPIVersionMinor() override;
	int GetPlugInVersionMajor() override;
	int GetPlugInVersionMinor() override;
	wxString GetCommonName() override;
	wxString GetShortDescription() override;
	wxString GetLongDescription() override;
	wxBitmap *GetPlugInBitmap() override;
	int GetToolbarToolCount(void) override;
	void OnToolbarToolCallback(int id) override;
	void SetDefaults(void) override;
	void SetPluginMessage(wxString &message_id, wxString &message_body) override;
	void SetActiveLegInfo(Plugin_Active_Leg_Info &leg_info) override;
	void UpdateAuiStatus(void) override;
	void LateInit(void) override;

	// AUI Manager events
	void OnPaneClose(wxAuiManagerEvent& event);

private:
	// AUI Manager
	wxAuiManager *auiManager;

	// Reference to the OpenCPN configuration file
	wxFileConfig *configSettings;

	// Reference to the OpenCPN window handle
	wxWindow *parentWindow;

	// Bitmap used for both the plugin and dialogs
	wxBitmap pluginBitmap;

	// Toolbar Id
	int autopilotToolbar;

	// Toolbar State
	bool autopilotDialogVisible;

	// OpenCPN NMEA 0183 Talker Id 
	wxString talkerId;

		// Autopilot Dialog 
	AutopilotDialog *autopilotDialog;

	// Event Handler for events received from the Autopilot dialog
	void OnDialogEvent(wxCommandEvent &event);

	// Retrieve route and waypoint names for UI display purposes
	wxString LookupWaypointName(wxString guid);
	wxString LookupRouteName(wxString guid);

	// Autopilots may need Keep Alive messages and Navigation Data
	wxTimer *oneSecondTimer;
	void OnTimerElapsed(wxEvent &event);

	// Used to determine when to revert the status display 
	wxDateTime updateDisplay;

	// For parsing NMEA 183 APB, MWV, RMB and XTE sentences
	NMEA0183 nmea183;

	// "New" way for handling NMEA 0183 sentences
	void HandleMWV(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_mwv;

	// NMEA 2000 Wind Speed and Direction
	void HandleN2K_130306(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_130306;

	// "New" way for navigation data, supercedes SetPositionFix
	void HandleNavData(ObservedEvt ev);
	std::shared_ptr<ObservableListener> listener_nav;

	// Normalize headings and wind angles
	double NormalizeHeading(double angle);

	double NormalizeWindAngle(double angle);

	// Compute NMEA 0183 checksum
	wxString ComputeChecksum(wxString sentence);

	// Send Keep Alive message every second
	void SendRaymarineKeepAlive();

	// Change Raymarine Heading
	void SetRaymarineHeading(double heading);

	// Quick & Dirty Engage Raymarine Autopilot
	void SetRaymarineAutopilot(AUTOPILOT_MODE state);

	// Autopilot address - Douwe hardcodes this to 204 (0xCC) ?
	int autopilotControllerAddress;

	// OCPN Network Interface
	DriverHandle GetNetworkInterface(std::string desiredProtocol);
	DriverHandle n2kNetworkHandle;

};
#endif 