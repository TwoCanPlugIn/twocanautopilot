// Copyright(C) 2022 - 2026 by Steven Adler
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
// 1.2.2 - 16/2/2026 - Bastardised for Raymarine & Shipmodul Miniplex Testing
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
		configSettings->SetPath(_T("/PlugIns/TwoCanAutopilot"));
		
		// Determine if the dialog was previously displayed
		configSettings->Read(_T("Visible"), &autopilotDialogVisible, false);

		// BUG BUG Douwe hardcodes this. Does the EV-1 not do the address claim dance
		configSettings->Read(_T("Address"), &autopilotControllerAddress, 204);

	}
	else {
		autopilotDialogVisible = false;
		autopilotControllerAddress = 204;
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
	
	// BUG BUG Not implemented in this bastardised test version
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

	// Find a NMEA 2000 Network Interface
	n2kNetworkHandle = GetNetworkInterface("nmea2000");
}

// OpenCPN is either closing down, or we have been disabled from the Preferences Dialog
bool AutopilotPlugin::DeInit(void) {
	Disconnect(wxEVT_AUTOPILOT_DIALOG_EVENT, wxCommandEventHandler(AutopilotPlugin::OnDialogEvent));
	auiManager->Disconnect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(AutopilotPlugin::OnPaneClose), NULL, this);
	auiManager->UnInit();
	auiManager->DetachPane(autopilotDialog);
	delete autopilotDialog;

	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/TwoCanAutopilot"));
		configSettings->Write(_T("Visible"), autopilotDialogVisible);
		// BUG BUG Not peristing the EV-1 address
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
		// Unsure whether the automatic generation of PGN 129282 (XTE) & 129284 (Nav data)
		// is consumed by the EV-1 is in Nav mode
		// Alternatively, could use Douwe's algorithm which calculates smoother course changes
		// SetRaymarineHeading(pInfo.Btw);
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
			// Is this necessary when the A/P is in Wjnd Mode.
			currentHeading = NormalizeHeading(currentHeading + (apparentWindAngle - desiredWindAngle));
			//SetRaymarineHeading(currentHeading);
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
					SetRaymarineAutopilot(AUTOPILOT_MODE::STANDBY);
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
					SetRaymarineAutopilot(AUTOPILOT_MODE::STANDBY);
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
					SetRaymarineAutopilot(AUTOPILOT_MODE::STANDBY);
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
					SetRaymarineAutopilot(AUTOPILOT_MODE::STANDBY);
				}
			}
		}
	}
}

DriverHandle AutopilotPlugin::GetNetworkInterface(std::string desiredProtocol) {

	assert(GetActiveDrivers().size() == 0);

	wxLogMessage("TwoCan Autopilot Plugin, Number of Active Drivers: %d", GetActiveDrivers().size());

	for (const auto& driver : GetActiveDrivers()) {
		const auto& attributes = GetAttributes(driver);
		// Debug Dump out all of the key value pairs
		for (auto it : attributes) {
			wxLogMessage("Debug: Key: %s, Value: %s", it.first, it.second);
		}
		// If none of the std::map entries have a protocol attribute, do the next for loop iteration
		if (attributes.find("protocol") == attributes.end())
			continue;
		wxLogMessage("Network Interface, Protocol: %s", attributes.at("protocol"));
		if (attributes.at("protocol") == desiredProtocol) {
			// Found our requested protocol
			
			// BUG BUG FFS, Regression, no I/O Direction for NMEA 20000
			//if (attributes.find("ioDirection") != attributes.end()) {
				// Found a driver that supports output
				//if ((attributes.at("ioDirection") == "IN/OUT") || (attributes.at("ioDirection") == "OUT")) {
				//}
			//}
			wxLogMessage("Network Interface, Using %s for %s", driver, desiredProtocol);
			return driver;
		}
	}
	wxLogMessage("TwoCan Autopilot Plugin, No driver found supporting %s", desiredProtocol);
	return "";
}


// Handle events from the dialog and generate Raymarine Autopilot commands
void AutopilotPlugin::OnDialogEvent(wxCommandEvent& event) {
	
	switch (event.GetId()) {
	case AUTOPILOT_MODE_CHANGED:
		
		autopilotMode = (AUTOPILOT_MODE)event.GetInt();
		// BUG BUG Simplify this shit
		switch (autopilotMode) {

		case AUTOPILOT_MODE::STANDBY:
			SetRaymarineAutopilot(AUTOPILOT_MODE::STANDBY);
			break;

		case AUTOPILOT_MODE::COMPASS:
			SetRaymarineAutopilot(AUTOPILOT_MODE::COMPASS);
			break;

		case AUTOPILOT_MODE::NAV:
			SetRaymarineAutopilot(AUTOPILOT_MODE::NAV);
			break;

		case AUTOPILOT_MODE::WIND:
			SetRaymarineAutopilot(AUTOPILOT_MODE::WIND);
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
		SendRaymarineKeepAlive();

		// Instead of parsing OCPN Messages such as OCPN_RTE_ACTIVATED could use
		// if (!GetActiveRouteGUID().IsEmpty()), to check if we have an active route/waypoint
	}
}

// Raymarine Keep Alive
void AutopilotPlugin::SendRaymarineKeepAlive() {
	std::vector<uint8_t> payload;
	
	// PGN 65384
	payload.clear();
	
	payload.push_back(0x3b);
	payload.push_back(0x9f);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);
	payload.push_back(0x00);


	auto sharedPointer = std::make_shared<std::vector<uint8_t>>(payload);
	CommDriverResult result = WriteCommDriverN2K(n2kNetworkHandle, 65384, autopilotControllerAddress,
		5, sharedPointer);
	if (result != RESULT_COMM_NO_ERROR) {
		wxLogMessage(_T("TwoCan Autopilot Plugin, Error sending Keep Alive, %s: %d"), n2kNetworkHandle.c_str(), result);
	}
}


// Change the heading
void AutopilotPlugin::SetRaymarineHeading(double value) {
	std::vector<uint8_t> payload;

	// PGN 126208 Group Function Command
	payload.push_back(0x01);

	// Commanded PGN
	// PGN 65360 (00FF50) Seatalk Heading
	payload.push_back(0x50);
	payload.push_back(0xFF);
	payload.push_back(0x00);

	// Reserved bits 0xF0 | 0x08 = Priority unchanged
	payload.push_back(0xF8);

	// Number of Parameter Pairs
	payload.push_back(0x03);

	// First Pair, Field 1 of PGN 65360 
	payload.push_back(0x01);
	// Manufacturer Code 0x073B == 1851
	payload.push_back(0x3b);
	payload.push_back(0x07);

	// Second Pair, Field 3 of PGN 65360
	payload.push_back(0x03);
	// Industry Code, 4 = Marine
	payload.push_back(0x04);

	// Third Pair, Field 4 of PGN 65360
	payload.push_back(0x04);
	// Heading, Convert to radians * 1e4
	unsigned short heading = DEGREES_TO_RADIANS(value) * 10000;
	payload.push_back(heading & 0xFF);
	payload.push_back((heading >> 8) & 0xFF);

	auto sharedPointer = std::make_shared<std::vector<uint8_t>>(payload);
	CommDriverResult result = WriteCommDriverN2K(n2kNetworkHandle, 126208, 
		autopilotControllerAddress,	5, sharedPointer);
	if (result != RESULT_COMM_NO_ERROR) {
		wxLogMessage(_T("TwoCan Autopilot, Error Changing Heading, %s: %d"), n2kNetworkHandle.c_str(), result);
	}
	
}

// Quick and Dirty Engage Autopilot
void AutopilotPlugin::SetRaymarineAutopilot(AUTOPILOT_MODE state) {
	std::vector<uint8_t> payload;

	// PGN 126208 Group Function Command
	payload.push_back(0x01);

	// Commanded PGN
	// PGN 65379 (0x00FF63) Seatalk Pilot Mode
	payload.push_back(0x63);
	payload.push_back(0xFF);
	payload.push_back(0x00);

	// Reserved bits 0xF0 | 0x08 = Priority unchanged
	payload.push_back(0xF8);

	// Number of Parameter Pairs
	// BUG BUG Perhaps a variadic or iterator list function ??
	payload.push_back(0x04);

	// First Pair, Field 1 of PGN 65379 
	payload.push_back(0x01);
	// Manufacturer Code 0x073B == 1851
	payload.push_back(0x3b);
	payload.push_back(0x07);

	// Second Pair, Field 3 of PGN 65379
	payload.push_back(0x03);
	// Industry Code, 4 = Marine
	payload.push_back(0x04);

	// Third Pair, Field 4 of PGN 65379
	payload.push_back(0x04);
	// Pilot Mode

	if (state == AUTOPILOT_MODE::STANDBY) {
		payload.push_back(0x00);
		payload.push_back(0x00);
	}
	if (state == AUTOPILOT_MODE::COMPASS) {
		payload.push_back(0x40);
		payload.push_back(0x00);
	}
	if (state == AUTOPILOT_MODE::NAV) {
		payload.push_back(0x80);
		payload.push_back(0x01);
	}
	if (state == AUTOPILOT_MODE::WIND) {
		payload.push_back(0x00);
		payload.push_back(0x01);
	}
	
	// Fourth Pair, Field 5 of PGN 65379
	payload.push_back(0x05);
	// Pilot Sub Mode 0xFFFF undefined
	payload.push_back(0xFF);
	payload.push_back(0xFF);

	auto sharedPointer = std::make_shared<std::vector<uint8_t>>(payload);
	CommDriverResult result = WriteCommDriverN2K(n2kNetworkHandle, 126208,
		autopilotControllerAddress, 5, sharedPointer);
	if (result != RESULT_COMM_NO_ERROR) {
		wxLogMessage(_T("TwoCan Autopilot, Error Changing Mode, %s: %d"), n2kNetworkHandle.c_str(), result);
	}
}