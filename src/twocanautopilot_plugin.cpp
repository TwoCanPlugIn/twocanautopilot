// Copyright(C) 2022 by Steven Adler
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
//
// NMEA2000® is a registered trademark of the National Marine Electronics Association
//
//
// Project: TwoCan Autopilot Plugin
// Description: Rudimentary control of Autopilot Computers
// Unit: Autopilot plugin implementation
// Owner: twocanplugin@hotmail.com
// Date: 30/06/2022
// Version History:
// 1.0 Initial Release
// 1.1 - 20/02/2025 - Code cleanup, Add Rudder Angle display and Alarm labels.
// 1.2 - 17/07/2025 - New dialog buttons, Updated OpenCPN Libs
// 1.2.1 - 23/10/2025 - Bastardised for Nautinect 
//

#include "twocanautopilot_plugin.h"

// The class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
	return new AutopilotPlugin(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
	delete p;
}

// Constructor
AutopilotPlugin::AutopilotPlugin(void *ppimgr) : opencpn_plugin_120(ppimgr),  wxEvtHandler() {
	
	// Load the plugin icon
	wxString bitmapFolder = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator()
		+ _T("images") + wxFileName::GetPathSeparator();

	pluginBitmap = GetBitmapFromSVGFile(bitmapFolder + "autopilot-toggled.svg", 32, 32);

	// Initialize Advanced User Interface Manager (AUI)
	auiManager = GetFrameAuiManager();

	// Start a one second timer to send keep alive messages to the autopilot
	// When OpenCPN is navigating, transmit navigation and cross track error messages
	oneSecondTimer = new wxTimer();
	oneSecondTimer->Bind(wxEVT_TIMER, &AutopilotPlugin::OnTimerElapsed, this);
	oneSecondTimer->Start(1000, wxTIMER_CONTINUOUS);
}

// Destructor
AutopilotPlugin::~AutopilotPlugin(void) {

	oneSecondTimer->Stop();
	oneSecondTimer->Unbind(wxEVT_TIMER, &AutopilotPlugin::OnTimerElapsed, this);
	delete oneSecondTimer;

}

int AutopilotPlugin::Init(void) {
	// Maintain a reference to the OpenCPN window to use as the parent
	parentWindow = GetOCPNCanvasWindow();

	// Maintain a reference to the OpenCPN configuration object 
	configSettings = GetOCPNConfigObject();

	// Load Configuration Settings
	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/Nautinect"));
		
		// Determine if the dialog was previously displayed
		configSettings->Read(_T("Visible"), &autopilotDialogVisible, false);

		// Any other Nauticnet settings to be persisted ??
		// Perhaps the heading source or calibration settings etc.

	}
	else {
		autopilotDialogVisible = false;
	}

	// Load toolbar icons
	wxString bitmapFolder = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator() 
		+ _T("images") + wxFileName::GetPathSeparator();
	
	wxString normalIcon = bitmapFolder + _T("autopilot-normal.svg");
	wxString toggledIcon = bitmapFolder + _T("autopilot-toggled.svg");
	wxString rolloverIcon = bitmapFolder + _T("autopilot-rollover.svg");

	// Insert the toolbar icons
	autopilotToolbar = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK, _("TwoCan Autopilot"), _T(""), NULL, -1, 0, this);

	// Instantiate the autopilot dialog
	autopilotDialog = new  AutopilotDialog(parentWindow, this);

	// Only enable GPS Mode if OpenCPN is following a route or steering to a waypoint
	autopilotDialog->EnableGPSMode(!GetActiveWaypointGUID().IsEmpty());
	
	// ToDo - Does nauticnet raise alarms?
	autopilotDialog->EnableAlarm(false);

	// Initialize dialog labels
	autopilotDialog->SetStatusLabel("Status:");

	// Listeners (for NMEA 0183 supercedes SetNMEASentence)
	// NMEA 0183 MWV Wind Sentence
	wxDEFINE_EVENT(EVT_183_MWV, ObservedEvt);
	NMEA0183Id id_mwv = NMEA0183Id("MWV");
	listener_mwv = std::move(GetListener(id_mwv, EVT_183_MWV, this));
	Bind(EVT_183_MWV, [&](ObservedEvt ev) {
		HandleMWV(ev);
		});

	// PGN 130306 Wind
	wxDEFINE_EVENT(EVT_N2K_130306, ObservedEvt);
	NMEA2000Id id_130306 = NMEA2000Id(130306);
	listener_130306 = std::move(GetListener(id_130306, EVT_N2K_130306, this));
	Bind(EVT_N2K_130306, [&](ObservedEvt ev) {
		HandleN2K_130306(ev);
		});


	// OpenCPN Core NavData (supercedes SetPositionFix)
	wxDEFINE_EVENT(EVT_NAV_DATA, ObservedEvt);
	listener_nav = std::move(GetListener(NavDataId(), EVT_NAV_DATA, this));
	Bind(EVT_NAV_DATA, [&](ObservedEvt ev) {
		HandleNavData(ev);
		});

	// Wire up the plugin event handler to receive events from the dialog
	Connect(wxEVT_AUTOPILOT_DIALOG_EVENT, wxCommandEventHandler(AutopilotPlugin::OnDialogEvent));

	// Notify OpenCPN what events we want to receive callbacks for
	return (WANTS_CONFIG | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_PLUGIN_MESSAGING | 
		USES_AUI_MANAGER | WANTS_NMEA_EVENTS | WANTS_LATE_INIT);
}

void AutopilotPlugin::LateInit(void) {
	// For some reason unbeknownst to me, previously the aui manager fails to wire up correctly
	// if done in the constructor or init. Seems to wire up correctly here though....

	// Load our dialog into the AUI Manager
	wxAuiPaneInfo paneInfo;
	paneInfo.Name(_T(PLUGIN_COMMON_NAME));
	paneInfo.Caption(_T(PLUGIN_COMMON_NAME));
	paneInfo.CloseButton(true);
	paneInfo.Float();
	paneInfo.Dockable(false);
	paneInfo.Show(autopilotDialogVisible);
	auiManager->AddPane(autopilotDialog, paneInfo);
	auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(AutopilotPlugin::OnPaneClose), NULL, this);
	auiManager->Update();
}

// OpenCPN is either closing down, or we have been disabled from the Preferences Dialog
bool AutopilotPlugin::DeInit(void) {
	Disconnect(wxEVT_AUTOPILOT_DIALOG_EVENT, wxCommandEventHandler(AutopilotPlugin::OnDialogEvent));
	auiManager->Disconnect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(AutopilotPlugin::OnPaneClose), NULL, this);
	auiManager->UnInit();
	auiManager->DetachPane(autopilotDialog);
	delete autopilotDialog;

	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/Nautinect"));
		configSettings->Write(_T("Visible"), autopilotDialogVisible);
		// Persist any other settings as need be
	}
	return true;
}

// Overridden OpenCPN methods

// Indicate what version of the OpenCPN Plugin API we support
int AutopilotPlugin::GetAPIVersionMajor() {
	return OCPN_API_VERSION_MAJOR;
}

int AutopilotPlugin::GetAPIVersionMinor() {
	return OCPN_API_VERSION_MINOR;
}

// The autopilot plugin version numbers. 
int AutopilotPlugin::GetPlugInVersionMajor() {
	return PLUGIN_VERSION_MAJOR;
}

int AutopilotPlugin::GetPlugInVersionMinor() {
	return PLUGIN_VERSION_MINOR;
}

// Descriptions for the autopilot plugin
wxString AutopilotPlugin::GetCommonName() {
	return _T(PLUGIN_COMMON_NAME);
}

wxString AutopilotPlugin::GetShortDescription() {
	return _T(PLUGIN_SHORT_DESCRIPTION);
}

wxString AutopilotPlugin::GetLongDescription() {
	return _T(PLUGIN_LONG_DESCRIPTION);
}

// Autopilot plugin icon
wxBitmap* AutopilotPlugin::GetPlugInBitmap() {
	return &pluginBitmap;
}

// We install one toolbar item
int AutopilotPlugin::GetToolbarToolCount(void) {
 return 1;
}

int AutopilotPlugin::GetToolbarItemId() { 
	return autopilotToolbar; 
}

void AutopilotPlugin::SetDefaults(void) {
	// Is called when the plugin is installed/enabled from the Plugin Manager dialog
	// Provides an opportunity to configure default values.
}

// UpdateAUI Status is invoked by OpenCPN when the saved AUI perspective is loaded
void AutopilotPlugin::UpdateAuiStatus(void) {
	auiManager->GetPane(_T(PLUGIN_COMMON_NAME)).Show(autopilotDialogVisible);
	auiManager->Update();
	SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
}

// Toggle the display of dialog as appropriate when the toolbar button is pressed
void AutopilotPlugin::OnToolbarToolCallback(int id) {
	if (id == autopilotToolbar) {
		autopilotDialogVisible = !autopilotDialogVisible;
		auiManager->GetPane(_T(PLUGIN_COMMON_NAME)).Show(autopilotDialogVisible);
		auiManager->Update();
		SetToolbarItemState(id, autopilotDialogVisible);
	}
}

// Keep the toolbar in synch with the pane state (user has closed the dialog from the "x" button)
void AutopilotPlugin::OnPaneClose(wxAuiManagerEvent& event) {
	wxAuiPaneInfo *paneInfo = event.GetPane();
	if (paneInfo->name == _T(PLUGIN_COMMON_NAME)) {
		autopilotDialogVisible = false;
		SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
	}
	else {
		event.Skip();
	}
}

// Generate the NMEA 0183 XOR checksum to be appended to an NMEA 0183 sentence
wxString AutopilotPlugin::ComputeChecksum(wxString sentence) {

	unsigned char calculatedChecksum = 0;
	for (wxString::const_iterator it = sentence.begin() + 1; it != sentence.end(); ++it) {
		calculatedChecksum ^= static_cast<unsigned char> (*it);
	}
	return(wxString::Format("%02X", calculatedChecksum));
}

// Handler for Navigation Data events (supercedes SetPositionFix)
void AutopilotPlugin::HandleNavData(ObservedEvt ev) {

	PluginNavdata navdata = GetEventNavdata(ev);
	currentHeading = navdata.hdt - navdata.var;

	if (autopilotMode == AUTOPILOT_MODE::COMPASS) {
		if (autopilotDialog != nullptr) {
			autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f (%.1f)",
				currentHeading, desiredHeading));
		}

		// ToDo Unsure if nauticnet autopilot needs a heading sent constantly or just once
		SetNauticnetHeading(desiredHeading);
	}
	else if (autopilotMode == AUTOPILOT_MODE::NAV) {
		
		desiredHeading = currentHeading;

		if (autopilotDialog != nullptr) {
			autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f (%.1f)",
				currentHeading, waypointHeading));
		}
	}
	else if (autopilotMode == AUTOPILOT_MODE::STANDBY) {
		
		desiredHeading = currentHeading;
		
		if (autopilotDialog != nullptr) {
			autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f",
				currentHeading));
		}
	}
}

// OpenCPN sends this every second when a route or waypoint is active
void AutopilotPlugin::SetActiveLegInfo(Plugin_Active_Leg_Info& pInfo) {
	
	waypointHeading = pInfo.Btw;

	if (autopilotMode == AUTOPILOT_MODE::NAV) {
		// Adjust autopilot heading to maintain the course to the waypoint
		// Doesn't compensate for drift, leeway etc. 
		// Alternatively, could use Douwe's algorithm which calculates smoother course changes
		SetNauticnetHeading(pInfo.Btw);
	}
}


// Parse NMEA 0183 Wind sentence
void AutopilotPlugin::HandleMWV(ObservedEvt ev) {

	NMEA0183Id id_183_mwv("MWV");
	NMEA0183 parserNMEA0183;
	wxString sentence = GetN0183Payload(id_183_mwv, ev);
	parserNMEA0183 << sentence;

	if (parserNMEA0183.Parse()) {
		double apparentWindAngle = parserNMEA0183.Mwv.WindAngle;

		// Only display wind angle when we are in Wind Mode
		if (autopilotMode == AUTOPILOT_MODE::WIND) {
			if (autopilotDialog != nullptr) {
				autopilotDialog->SetHeadingLabel(wxString::Format("Wind Angle: %.1f (%.1f)",
					NormalizeWindAngle(apparentWindAngle), 
					NormalizeWindAngle(desiredWindAngle)));
			}

			// Calculate new heading to maintain desired Wind Angle
			// ToDo Verify. Also what about tack & gybe ??
			currentHeading = NormalizeHeading(currentHeading + (apparentWindAngle - desiredWindAngle));
			SetNauticnetHeading(currentHeading);
		}
		else {
			// Persist the Apparent Wind Angle for when wind mode is engaged 
			desiredWindAngle = apparentWindAngle;
		}
	}
}

// Parse NMEA 2000 Wind message
void AutopilotPlugin::HandleN2K_130306(ObservedEvt ev) {

	NMEA2000Id id_130306(130306);
	std::vector<uint8_t> payload = GetN2000Payload(id_130306, ev);
	unsigned char sid;
	double windSpeed;
	double windAngle;
	tN2kWindReference windReferenceType;

	if (ParseN2kPGN130306(payload, sid, windSpeed, windAngle, windReferenceType)) {
		// Convert from m/s and radians to OpenCPN's core units
		// apparentWindSpeed = fromUsrSpeed_Plugin(windSpeed, 3);
		double apparentWindAngle = windAngle * 180 / M_PI;

		// ToDo
	}
}


// ToDo Should parse Rudder Sensor Angle. Either RSA sentence or PGN 127245
// if (autopilotDialog != nullptr) {
//		autopilotDialog->DrawRudderAngle(rudderAngle);
// }


// Receive OpenCPN Plugin Messages.
// OpenCPN sends messages to indicate if a route or waypoint has been activated.
void AutopilotPlugin::SetPluginMessage(wxString &message_id, wxString &message_body) {

	wxJSONReader reader;
	wxJSONWriter writer;
	wxJSONValue root;

	if (reader.Parse(message_body, &root) > 0) {
		// Save the erroneous json text for debugging
		wxLogMessage("TwoCan Autopilot, JSON Error in following message: %s", message_id);
		wxLogMessage("%s", message_body);
		wxArrayString jsonErrors = reader.GetErrors();
		for (auto it : jsonErrors) {
			wxLogMessage(it);
		}
		return;
	}
	else {


		if (message_id == _T("OCPN_RTE_ACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif
			// Note to self, in OpenCPN 5.12 there are new API's to simplify this
			// GetActiveRouteGuid
			// ToDo Need to look up start and ending waypoint names
			
			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel(wxString::Format("Route: %s", 
					LookupRouteName(root["GUID"].AsString())));
				autopilotDialog->EnableGPSMode(true);
			}
		}

		else if (message_id == _T("OCPN_RTE_DEACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Route: Deactivated");
				updateDisplay = wxDateTime::Now();
				autopilotDialog->EnableGPSMode(false);
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					SetNautecnetAutopilot(false);
				}
			}
		}

		else if (message_id == _T("OCPN_RTE_ENDED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Route: Complete");
				updateDisplay = wxDateTime::Now();
				autopilotDialog->EnableGPSMode(false);
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					SetNautecnetAutopilot(false);
				}
			}
		}

		else if (message_id == _T("OCPN_WPT_ACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel(wxString::Format("Waypoint: %s", 
					LookupWaypointName(GetActiveWaypointGUID())));
				autopilotDialog->EnableGPSMode(true);
			}
		}

		else if (message_id == _T("OCPN_WPT_DEACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif
			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Waypoint: Deactivated");
				updateDisplay = wxDateTime::Now();
				autopilotDialog->EnableGPSMode(false);
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					SetNautecnetAutopilot(false);
				}
			}
		}

		else if (message_id == _T("OCPN_WPT_ARRIVED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif 
			if (root.HasMember("GUID_Next_WP")) {

				// ToDo Should there be a confirm dialog ?

				if (autopilotDialog != nullptr) {
					autopilotDialog->SetStatusLabel(wxString::Format("Waypoint: %s", 
						LookupWaypointName(GetActiveWaypointGUID())));
					autopilotDialog->EnableGPSMode(true);
				}
			}
			else {
				if (autopilotDialog != nullptr) {
					autopilotDialog->SetStatusLabel(wxString::Format("Arrived: %s",
					LookupWaypointName(root["GUID_WP_arrived"].AsString())));
					autopilotDialog->EnableGPSMode(false);
					SetNautecnetAutopilot(false);
				}
			}
		}
	}
}

// Handle events from the dialog and generate Nautinect Autopilot commands
void AutopilotPlugin::OnDialogEvent(wxCommandEvent& event) {
	
	switch (event.GetId()) {
	case AUTOPILOT_MODE_CHANGED:
		
		autopilotMode = (AUTOPILOT_MODE)event.GetInt();
		// ToDo Replace with if else statement
		switch (autopilotMode) {

		case AUTOPILOT_MODE::STANDBY:
			SetNautecnetAutopilot(false);
			break;

		case AUTOPILOT_MODE::COMPASS:
			SetNautecnetAutopilot(true);
			break;

		case AUTOPILOT_MODE::NAV:
			SetNautecnetAutopilot(true);
			break;

		case AUTOPILOT_MODE::WIND:
			SetNautecnetAutopilot(true);
			break;
		}
		
		break;

	case AUTOPILOT_HEADING_CHANGED:

		switch (autopilotMode) {
		case AUTOPILOT_MODE::COMPASS:
			desiredHeading = NormalizeHeading(desiredHeading + event.GetInt());
			break;

		case AUTOPILOT_MODE::WIND:
			desiredWindAngle = NormalizeWindAngle(desiredWindAngle + event.GetInt());
			break;

		case AUTOPILOT_MODE::NAV:
			// ToDo What do we do when we change course when in Nav mode
			// Is this "dodging", in which case should the mode change to COMPASS
			break;
		}

		break;
	}
}


// Used to get the name of an active Waypoint
wxString AutopilotPlugin::LookupWaypointName(wxString guid) {
	PlugIn_Waypoint waypoint;
	GetSingleWaypoint(guid, &waypoint);
	return waypoint.m_MarkName;
}

// Used to get the name of the active route
wxString AutopilotPlugin::LookupRouteName(wxString guid) {
	std::unique_ptr<PlugIn_Route> activeRoute;
	activeRoute = GetRoute_Plugin(guid);
	return activeRoute->m_NameString;
}

// Headings are between 0 to 360
double AutopilotPlugin::NormalizeHeading(double heading) {
	double angle = fmod(heading, 360);
	return angle < 0 ? angle += 360 : angle;
}

// Wind angles are between -180 to 180
double AutopilotPlugin::NormalizeWindAngle(double angle) {
	if (angle > 180) {
		angle -= 360;
	}
	if (angle < -180) {
		angle += 360;
	}
	return angle;
}

// One Second Timer 
// For other autopilots used to generate Navigation, Route and Keep Alive messages
void AutopilotPlugin::OnTimerElapsed(wxEvent& event) {

	if (oneSecondTimer->IsRunning()) {

		// Update the UI, just in case we've stopped navigating, no need to display an erroneous string
		if (wxDateTime::Now() > (updateDisplay + wxTimeSpan::Seconds(5))) {
			if (GetActiveRouteGUID().IsEmpty()) {
				autopilotDialog->SetStatusLabel("Status: ");
			}
		}

		// Send Keep Alive messages
		// Does nautinect have a keep alive message?

		// Instead of parsing OCPN Messages such as OCPN_RTE_ACTIVATED could use
		// if (!GetActiveRouteGUID().IsEmpty()), to check if we have an active route/waypoint
	}
}

// For these two functions, instead of PushNMEABuffer, consider the "new" WriteCommDriver
void AutopilotPlugin::SetNauticnetHeading(double heading) {
	wxString nauticnetSentence = wxString::Format("$APCMD,2,%d",
		static_cast<int>(heading));
	wxString checksum = ComputeChecksum(nauticnetSentence);
	nauticnetSentence.Append("*");
	nauticnetSentence.Append(checksum);
	nauticnetSentence.Append("\r\n");
	PushNMEABuffer(nauticnetSentence);
}

void AutopilotPlugin::SetNautecnetAutopilot(bool state) {
	// ToDo heading source is hardcoded
	wxString nauticnetSentence = wxString::Format("$APCMD,%s",
		state ? "3,0" : "4");
	wxString checksum = ComputeChecksum(nauticnetSentence);
	nauticnetSentence.Append("*");
	nauticnetSentence.Append(checksum);
	nauticnetSentence.Append("\r\n");
	PushNMEABuffer(nauticnetSentence);
}