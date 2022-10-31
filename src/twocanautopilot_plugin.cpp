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
// Description: Rudimentary control of Autopilot Computers (via TwoCan plugin)
// Unit: Autopilot plugin implementation
// Owner: twocanplugin@hotmail.com
// Date: 30/06/2022
// Version History:
// 1.0 Initial Release
//

#include "twocanautopilot_plugin.h"
#include "twocanautopilot_icon.h"

// The class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
	return new AutopilotPlugin(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
	delete p;
}

// Constructor
AutopilotPlugin::AutopilotPlugin(void *ppimgr) : opencpn_plugin_117(ppimgr),  wxEvtHandler() {
	
	// Load the plugin icon
	// refer to twocanautopilot_icons.cpp
	initialize_images();

	// Initialize Advanced User Interface Manager (AUI)
	auiManager = GetFrameAuiManager();

	// Structure used to aggregate data necessary for configuring the autopilot to follow a route
	navigationData = {};
	navigationData.navigationHalted = true;


	// Start a one second timer to send keep alive messages to the autopilot
	// and when in GPS mode to send navigation and cross track error messages
	oneSecondTimer = new wxTimer();
	oneSecondTimer->Bind(wxEVT_TIMER, &AutopilotPlugin::OnTimerElapsed, this);
	oneSecondTimer->Start(1000, wxTIMER_CONTINUOUS);
}

// Destructor
AutopilotPlugin::~AutopilotPlugin(void) {
	delete pluginIcon;

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
		bool autopilotSettingsValid;
		configSettings->SetPath(_T("/PlugIns/TwoCan"));
		configSettings->Read(_T("Autopilot"), &autopilotSettingsValid, false);
		if (!autopilotSettingsValid) {
			wxLogMessage(_T("TwoCan Autopilot, Invalid or missing autopilot configuration"));
		}
		configSettings->SetPath(_T("/PlugIns/TwoCanAutopilot"));
		configSettings->Read(_T("Visible"), &autopilotDialogVisible, false);
	}
	else {
		autopilotDialogVisible = false;
	}


	// Load toolbar icons
	wxString shareLocn = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator() 
		+ _T("images") + wxFileName::GetPathSeparator();
	
	wxString normalIcon = shareLocn + _T("autopilot-normal.svg");
	wxString toggledIcon = shareLocn + _T("autopilot-toggled.svg");
	wxString rolloverIcon = shareLocn + _T("autopilot-rollover.svg");

	// Insert the toolbar icons
	autopilotToolbar = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK, _("TwoCan Autopilot"), _T(""), NULL, -1, 0, this);

	// Instantiate the autopilot dialog
	autopilotDialog = new  AutopilotDialog(parentWindow, this);

	// Wire up the event handler to receive events from the dialog
	Connect(wxEVT_AUTOPILOT_DIALOG_EVENT, wxCommandEventHandler(AutopilotPlugin::OnDialogEvent));

	// Notify OpenCPN what events we want to receive callbacks for
	return (WANTS_CONFIG | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_PLUGIN_MESSAGING | USES_AUI_MANAGER | WANTS_LATE_INIT | WANTS_NMEA_EVENTS | WANTS_NMEA_SENTENCES);
}

void AutopilotPlugin::LateInit(void) {
	// For some reason unbeknownst to me, the aui manager fails to wire up correctly if done
	// in the constructor or init. Seems to wire up correctly here though....

	// Load our dialog into the AUI Manager
	wxAuiPaneInfo paneInfo;
	paneInfo.Name(_T("TwoCan Autopilot Controller"));
	paneInfo.Caption("TwoCan Autopilot Controller Plugin for OpenCPN");
	paneInfo.CloseButton(true);
	paneInfo.Float();
	paneInfo.Dockable(false);
	paneInfo.Show(autopilotDialogVisible);
	auiManager->AddPane(autopilotDialog, paneInfo);
	auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(AutopilotPlugin::OnPaneClose), NULL, this);
	auiManager->Update();
	// Attempt to disable the selection of Navigation Mode if no route is active
	autopilotDialog->EnableGPSMode(false);
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
// 32x32 pixel PNG file, use pgn2wx.pl perl script
wxBitmap* AutopilotPlugin::GetPlugInBitmap() {
		return pluginIcon;
}

// We install one toolbar item
int AutopilotPlugin::GetToolbarToolCount(void) {
 return 1;
}

int AutopilotPlugin::GetToolbarItemId() { 
	return autopilotToolbar; 
}

void AutopilotPlugin::SetDefaults(void) {
	// Is called when the plugin is enabled
	// Anything to do ?
}

// UpdateAUI Status is invoked by OpenCPN when the saved AUI perspective is loaded
void AutopilotPlugin::UpdateAuiStatus(void) {
	auiManager->GetPane(_T("TwoCan Autopilot Controller")).Show(autopilotDialogVisible);
	auiManager->Update();
	SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
}

// Toggle the display of dialog as appropriate when the toolbar button is pressed
void AutopilotPlugin::OnToolbarToolCallback(int id) {
	if (id == autopilotToolbar) {
		autopilotDialogVisible = !autopilotDialogVisible;
		auiManager->GetPane(_T("TwoCan Autopilot Controller")).Show(autopilotDialogVisible);
		auiManager->Update();
		SetToolbarItemState(id, autopilotDialogVisible);
	}
}

// Keep the toolbar in synch with the pane state (user has closed the dialog from the "x" button)
void AutopilotPlugin::OnPaneClose(wxAuiManagerEvent& event) {
	wxAuiPaneInfo *paneInfo = event.GetPane();
	if (paneInfo->name == _T("TwoCan Autopilot Controller")) {
		autopilotDialogVisible = false;
		SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
	}
	else {
		event.Skip();
	}
}

void AutopilotPlugin::SetNMEASentence(wxString &sentence) {

	
	// Parse the received NMEA 183 sentence
	nmea183 << sentence;

	if (nmea183.PreParse()) {

		if (autopilotMode == AUTOPILOT_MODE::WIND) {
			// Only display wind angle when we are in Wind Mode
			if (nmea183.LastSentenceIDReceived == _T("MWV")) {
				if (nmea183.Parse()) {
					if (nmea183.Mwv.IsDataValid == NTrue) {
						if (nmea183.Mwv.Reference == _T("R")) { // apparent wind (relative to boat)
							autopilotDialog->SetHeadingLabel(wxString::Format("Wind Angle: %.1f", 
								nmea183.Mwv.WindAngle > 180 ? -(180.0 - (nmea183.Mwv.WindAngle - 180.0)) : nmea183.Mwv.WindAngle));
						}
					}
				}
			}
		}

		else {
			// BUG BUG Should this be configurable or is this hardcoded for OCPN internally ??
			if (sentence.StartsWith("$EC")) {

				OutputDebugString(sentence);

				// If we have an active route or navigating to a waypoint
				// Need to send info to the autopilot
				if (navigationData.navigationHalted == false) {
					if (nmea183.LastSentenceIDReceived == _T("RMB")) {
						if (nmea183.Parse()) {
							if (nmea183.Rmb.IsDataValid == NTrue) {

								navigationData.currentBearing = 10000 * DEGREES_TO_RADIANS(nmea183.Rmb.BearingToDestinationDegreesTrue);

								// BUG BUG Could populate from the OCPN Plugn Messages and Lookup Waypoint Function
								navigationData.destinationLatitude = nmea183.Rmb.DestinationPosition.Latitude.Latitude * 1e7;
								if (nmea183.Rmb.DestinationPosition.Latitude.Northing == NORTHSOUTH::South) {
									navigationData.destinationLatitude = -navigationData.destinationLatitude;
								}

								navigationData.destinationLongitude = nmea183.Rmb.DestinationPosition.Longitude.Longitude * 1e7;
								if (nmea183.Rmb.DestinationPosition.Longitude.Easting == EASTWEST::West) {
									navigationData.destinationLongitude = -navigationData.destinationLongitude;
								}

								// BUG BUG is the sign of cross track error correct ??
								navigationData.crossTrackError = 100 * nmea183.Rmb.CrossTrackError / CONVERT_METRES_NAUTICAL_MILES;

								// BUG BUG just to confirm sign of XTE
								OutputDebugString(wxString::Format("XTE: %d\n", navigationData.crossTrackError));

								if (nmea183.Rmb.DirectionToSteer == LEFTRIGHT::Left) {
									navigationData.crossTrackError = -navigationData.crossTrackError;
								}
								navigationData.waypointClosingVelocity = 100 * nmea183.Rmb.DestinationClosingVelocityKnots / CONVERT_MS_KNOTS;

								navigationData.distanceToWaypoint = (unsigned int)(10 * nmea183.Rmb.RangeToDestinationNauticalMiles / CONVERT_METRES_NAUTICAL_MILES);

							}

						}
					}
					if (nmea183.LastSentenceIDReceived == _T("APB")) {
						if (nmea183.Parse()) {
							// BUG BUG Units. These are True, but what if RMB is Magnetic ??
							navigationData.originalBearing = 10000 * DEGREES_TO_RADIANS(nmea183.Apb.BearingOriginToDestination);
							navigationData.arrivalCircleEntered = nmea183.Apb.IsArrivalCircleEntered;
							navigationData.perpendicularCrossed = nmea183.Apb.IsPerpendicular;
						}
					}
				}
			}
		}
	}
}

// Receive OpenCPN Plugin Messages.
void AutopilotPlugin::SetPluginMessage(wxString &message_id, wxString &message_body) {

	// Brief description of the JSON schema used by TwoCan Autopilot
	// Some are send & receive, some are receive only and some send only

	// root["autopilot"]["mode"] 0 standby | 1 heading (compass)  | 2 nav (gps) | 3 wind | 4 no drift (not implemented)
	// root["autopilot"]["model"] 0 Garmin, 1 Raymarine | 2 Simrad AC12| | 3 Navico NAC 3 | 4 Furuno
	// root["autopilot"]["heading"] in degrees
	// root["autopilot"]["windangle"] in +/- degrees - port, + starboard
	// rooy["rudderangle"] in +/- degrees
	// root["autopilot"]["xte"] // BUG BUG Should use the same units as selected by the user 
	// root["autopilot"]["bearing"]
	// root["autopilot"]["alarm"] some text description
		
	wxJSONReader reader;
	wxJSONWriter writer;
	wxJSONValue root;

	if (reader.Parse(message_body, &root) > 0) {
		// Save the erroneous json text for debugging
		wxLogMessage("TwoCan Autopilot, JSON Error in following text:");
		wxLogMessage("%s", message_body);
		wxArrayString jsonErrors = reader.GetErrors();
		for (auto it : jsonErrors) {
			wxLogMessage(it);
		}
		return;
	}
	else {

		if (message_id == _T("TWOCAN_AUTOPILOT_RESPONSE")) {

			// Update dialog to reflect actual autopilot status
			if (root["autopilot"].HasMember("mode")) {
				autopilotDialog->SetMode((AUTOPILOT_MODE)root["autopilot"]["mode"].AsInt());
				if ((AUTOPILOT_MODE)root["autopilot"]["mode"].AsInt() == AUTOPILOT_MODE::NAV) {
					autopilotDialog->EnableGPSMode(true);
				}
				else {
					autopilotDialog->EnableGPSMode(false);
				}
			}

			// Heading actually comes from this plugin from SetPositionFixEx
			// BUG BUG Following is redundant
			if ((root["autopilot"].HasMember("heading")) && ((autopilotMode == AUTOPILOT_MODE::COMPASS) ||
				(autopilotMode == AUTOPILOT_MODE::NAV))) {
				int heading = root["autopilot"]["heading"].AsInt();
				//autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f", heading));
			}

			// Wind Angle actually comes from processing of MWV NMEA 183 sentence
			// BUG BUG Following is redundant
			if ((root["autopilot"].HasMember("windangle")) && (autopilotMode == AUTOPILOT_MODE::WIND)) {
				int windAngle = root["autopilot"]["windangle"].AsInt();
				//autopilotDialog->SetHeadingLabel(wxString::Format("Wind Angle: %.1f", windAngle));
			}

			if (root["autopilot"].HasMember("alarm")) {
				wxString alarm = root["autopilot"]["alarm"].AsString();
				autopilotDialog->SetAlarmLabel(alarm);
			}

			if (root["autopilot"].HasMember("rudderangle")) {
				int rudderAngle = root["autopilot"]["rudderangle"].AsInt();
				// Need a linear meter to display rudder angle
				//autopilotDialog->SetHeadingLabel(wxString::Format("Rudder Angle: %d", rudderAngle);
			}
		}

		else if (message_id == _T("OCPN_RTE_ACTIVATED")) {
			OutputDebugStringA(message_id.ToAscii().data());
			OutputDebugStringA(message_body.ToAscii().data());

			navigationData.routeId = strtoul(root["GUID"].AsString().Mid(0, 6), NULL, 16);
			navigationData.routeName = LookupRouteName(root["GUID"].AsString());
			if (navigationData.routeName.size() == 0) {
				navigationData.routeName = "Unamed Route";
			}

			navigationData.navigationHalted = false;
			
			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel(wxString::Format("Route: %s", navigationData.routeName));
				autopilotDialog->EnableGPSMode(true);
			}
		}

		else if (message_id == _T("OCPN_RTE_DEACTIVATED")) {
			OutputDebugStringA(message_id.ToAscii().data());
			OutputDebugStringA(message_body.ToAscii().data());

			navigationData.routeName.clear();
			navigationData.navigationHalted = true;

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Route: Deactivated");
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->EnableGPSMode(false);
				}
			}
		}

		else if (message_id == _T("OCPN_RTE_ENDED")) {
			OutputDebugStringA(message_id.ToAscii().data());
			OutputDebugStringA(message_body.ToAscii().data());

			navigationData.routeName.clear();
			// BUG BUG Confirm if following are reflected in APNB or RMB sentence
			navigationData.arrivalCircleEntered = true;
			navigationData.perpendicularCrossed = true;
			navigationData.navigationHalted = true;

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Route: Complete");
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->EnableGPSMode(false);
				}
			}
		}

		else if (message_id == _T("OCPN_WPT_ACTIVATED")) {
			OutputDebugStringA(message_id.ToAscii().data());
			OutputDebugStringA(message_body.ToAscii().data());

			navigationData.navigationHalted = false;
			navigationData.originId = 0;
			navigationData.originName = "Start"; // BUG BUG Can we determine if there is a starting position name

			navigationData.destinationName = LookupWaypoint(root["GUID"].AsString()).m_MarkName;
			if (navigationData.destinationName.size() == 0) {
				navigationData.destinationName = "End";
			}
			navigationData.destinationId = strtoul(root["GUID"].AsString().Mid(0, 6), NULL, 16);

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel(wxString::Format("Waypoint: %s", navigationData.destinationName));
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->EnableGPSMode(true);
				}
			}

		}

		else if (message_id == _T("OCPN_WPT_DEACTIVATED")) {
			OutputDebugStringA(message_id.ToAscii().data());
			OutputDebugStringA(message_body.ToAscii().data());

			navigationData.navigationHalted = true;
			navigationData.destinationName.clear();

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Waypoint: Deactivated");
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->EnableGPSMode(false);
				}
			}
		}

		else if (message_id == _T("OCPN_WPT_ARRIVED")) {
			OutputDebugStringA(message_id.ToAscii().data());
			OutputDebugStringA(message_body.ToAscii().data());

			if (root.HasMember("GUID_Next_WP")) {

				// Update the origin and destination waypoints
				navigationData.originId = navigationData.destinationId;
				navigationData.destinationId = strtoul(root["GUID_Next_WP"].AsString().Mid(0, 6), NULL, 16);
				navigationData.originName = navigationData.destinationName;
				navigationData.destinationName = LookupWaypoint(root["GUID_Next_WP"].AsString()).m_MarkName;
				if (navigationData.destinationName.size() == 0) {
					navigationData.destinationName = "Unamed Waypoint";
				}

				if (autopilotDialog != nullptr) {
					autopilotDialog->SetStatusLabel(wxString::Format("Waypoint: %s", navigationData.destinationName));
					if (autopilotMode == AUTOPILOT_MODE::NAV) {
						autopilotDialog->EnableGPSMode(true);
					}
				}
			}
			else {
				// No next waypoint, navigation has ended
				navigationData.navigationHalted = true;
				navigationData.arrivalCircleEntered = true;
				navigationData.perpendicularCrossed = true;

				if (autopilotDialog != nullptr) {
					// Wouldn't his just be the destinationName ??
					autopilotDialog->SetStatusLabel(wxString::Format("Arrived: %s",
						LookupWaypoint(root["GUID_WP_arrived"].AsString()).m_MarkName));
					if (autopilotMode == AUTOPILOT_MODE::NAV) {
						autopilotDialog->EnableGPSMode(false);
					}
				}
			}
		}
	}
}

// Send XTE and Bearing data TwoCan plugin to generate PGN 127283 & 127284 messages.
// BUG BUG Not used as data is derived from NMEA183 APB and RMB sentences
// BUG BUG Not enough info is avalable in Plugin_Active_Leg_Info anyway
void AutopilotPlugin::SetActiveLegInfo(Plugin_Active_Leg_Info &pInfo) {
}

// Update our heading if not in Wind Mode
void AutopilotPlugin::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix) {
	if (autopilotDialog != nullptr) {
		if (autopilotMode != AUTOPILOT_MODE::WIND) {
			autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f", pfix.Hdm));
		}
	}
}

// Used to generate PGN 129285
PlugIn_Waypoint AutopilotPlugin::LookupWaypoint(wxString guid) {
	PlugIn_Waypoint waypoint;
	GetSingleWaypoint(guid, &waypoint);
	return waypoint;
}

// Used to generate PGN 129285
wxString AutopilotPlugin::LookupRouteName(wxString guid) {
	std::unique_ptr<PlugIn_Route> activeRoute;
	activeRoute = GetRoute_Plugin(guid);
	return activeRoute->m_NameString;
}

// One Second Timer used to send XTE, Navigation, Route and Keep Alive messages
void AutopilotPlugin::OnTimerElapsed(wxEvent &event) {
	// BUG BUG Conundrum, Do I construct the PGN's hhere or in TwoCan plugin ?? How to refactor TwoCanEncoder ??
	// BUG BUG Need an isRunning Flag in case the rug is pulled from under us

	wxString message_body;
	wxJSONValue root;
	wxJSONWriter writer;
	
	// Construct PGN 129283,129284 & 129285 Messages
	if (navigationData.navigationHalted == false) {

		// XTE
		root.Clear();
		root["nmea2000"]["pgn"] = 129283;
		root["nmea2000"]["source"] = 7; // BUG BUG Need to get our network address !!
		root["nmea2000"]["destination"] = 255;
		root["nmea2000"]["priority"] = 7;
		root["nmea2000"]["dlc"] = 8;

		root["nmea2000"]["data"][0] = 0xA0; // Secuenc Id
		// xte Mode set to 0 = Autonomous Mode, 0x30 is reserved
		root["nmea2000"]["data"][1] = (0 & 0x0F) | 0x30 | ((navigationData.navigationHalted << 6) & 0xC0);
		root["nmea2000"]["data"][2] = navigationData.crossTrackError & 0xFF;
		root["nmea2000"]["data"][3] = (navigationData.crossTrackError >> 8) & 0xFF;
		root["nmea2000"]["data"][4] = (navigationData.crossTrackError >> 16) & 0xFF;
		root["nmea2000"]["data"][5] = (navigationData.crossTrackError >> 24) & 0xFF;
		root["nmea2000"]["data"][6] = 0xFF;
		root["nmea2000"]["data"][7] = 0xFF;

		writer.Write(root, message_body);
		SendPluginMessage(_T("TWOCAN_TRANSMIT_FRAME"), message_body);

		// Navigation
		/*
		SID: 80
Distance: 2.09
Bearing Reference: 0
Perpendicular Crossed 0
Circle Entered: 0
Calculation Type: 0
Days: 19268
Seconds: 85482
Time (UTC):   03/10/2022 23:44:42
Bearing Origin: 16.66
Bearing Position: 16.36
Origin ID: 0
Destination ID: 1
Latitude: 39 54.44
Longitude: 3 5.14
Waypoint Closing Velocity: 0.040000

		*/
		root.Clear();

		root["nmea2000"]["pgn"] = 129284;
		root["nmea2000"]["source"] = 7; // BUG BUG Need to get our network address !!
		root["nmea2000"]["destination"] = 255;
		root["nmea2000"]["priority"] = 7;
		root["nmea2000"]["dlc"] = 34;

		root["nmea2000"]["data"][0] = 0xA0; // sequence Id);

		root["nmea2000"]["data"][1] = navigationData.distanceToWaypoint & 0xFF;
		root["nmea2000"]["data"][2] = (navigationData.distanceToWaypoint >> 8) & 0xFF;
		root["nmea2000"]["data"][3] = (navigationData.distanceToWaypoint >> 16) & 0xFF;
		root["nmea2000"]["data"][4] = (navigationData.distanceToWaypoint >> 24) & 0xFF;

		// byte bearingRef True = 0, Magnetic = 1
		// byte perpendicularCrossed No = 0
		// byte circleEntered  No = 0
		// byte calculationType Rhumb Line = 0, Great Circle = 1

		root["nmea2000"]["data"][5] = navigationData.bearingReference | (navigationData.perpendicularCrossed << 2)
			| (navigationData.arrivalCircleEntered << 4) | (0x0 << 6);

		// BUG BUG If this is ETA, need to calculate, Time = Distance / Speed
		wxDateTime epochTime((time_t)0);
		wxDateTime now = wxDateTime::Now();
		wxTimeSpan diff;
		unsigned short daysSinceEpoch;
		unsigned int secondsSinceMidnight;
		// Calculate the ETA
		if (navigationData.waypointClosingVelocity != 0) {
			diff += (navigationData.distanceToWaypoint / navigationData.waypointClosingVelocity); // Note in cm/s
			now.Add(diff);

			// Convert to Epoch Time
			diff = now - epochTime;

			daysSinceEpoch = diff.GetDays();
			secondsSinceMidnight = ((diff.GetSeconds() - (daysSinceEpoch * 86400)).GetValue()) * 10000;
		}
		else {
			daysSinceEpoch = USHRT_MAX;
			secondsSinceMidnight = UINT_MAX;
		}

		root["nmea2000"]["data"][6] = secondsSinceMidnight & 0xFF;
		root["nmea2000"]["data"][7] = (secondsSinceMidnight >> 8) & 0xFF;
		root["nmea2000"]["data"][8] = (secondsSinceMidnight >> 16) & 0xFF;
		root["nmea2000"]["data"][9] = (secondsSinceMidnight >> 24) & 0xFF;

		root["nmea2000"]["data"][10] = daysSinceEpoch & 0xFF;
		root["nmea2000"]["data"][11] = (daysSinceEpoch >> 8) & 0xFF;

		root["nmea2000"]["data"][12] = navigationData.originalBearing & 0xFF;
		root["nmea2000"]["data"][13] = (navigationData.originalBearing >> 8) & 0xFF;

		root["nmea2000"]["data"][14] = navigationData.currentBearing & 0xFF;
		root["nmea2000"]["data"][15] = (navigationData.currentBearing >> 8) & 0xFF;

		// B&G Just uses 0 and 1 for Origin and Destination Id
		root["nmea2000"]["data"][16] = 0;// navigationData.originId & 0xFF;
		root["nmea2000"]["data"][17] = 0;// (navigationData.originId >> 8) & 0xFF;
		root["nmea2000"]["data"][18] = 0;// (navigationData.originId >> 16) & 0xFF;
		root["nmea2000"]["data"][19] = 0;// (navigationData.originId >> 24) & 0xFF;

		root["nmea2000"]["data"][20] = 1;// navigationData.destinationId & 0xFF;
		root["nmea2000"]["data"][21] = 0;// (navigationData.destinationId >> 8) & 0xFF;
		root["nmea2000"]["data"][22] = 0;// (navigationData.destinationId >> 16) & 0xFF;
		root["nmea2000"]["data"][23] = 0;// (navigationData.destinationId >> 24) & 0xFF;

		root["nmea2000"]["data"][24] = navigationData.destinationLatitude & 0xFF;
		root["nmea2000"]["data"][25] = (navigationData.destinationLatitude >> 8) & 0xFF;
		root["nmea2000"]["data"][26] = (navigationData.destinationLatitude >> 16) & 0xFF;
		root["nmea2000"]["data"][27] = (navigationData.destinationLatitude >> 24) & 0xFF;

		root["nmea2000"]["data"][28] = navigationData.destinationLongitude & 0xFF;
		root["nmea2000"]["data"][29] = (navigationData.destinationLongitude >> 8) & 0xFF;
		root["nmea2000"]["data"][30] = (navigationData.destinationLongitude >> 16) & 0xFF;
		root["nmea2000"]["data"][31] = (navigationData.destinationLongitude >> 24) & 0xFF;

		root["nmea2000"]["data"][32] = navigationData.waypointClosingVelocity & 0xFF;
		root["nmea2000"]["data"][33] = (navigationData.waypointClosingVelocity >> 8) & 0xFF;

		writer.Write(root, message_body);
		SendPluginMessage(_T("TWOCAN_TRANSMIT_FRAME"), message_body);

		// Route
		/*
		Route Name Length: 3
RPS: 65535
Items: 2
Database Version: 65535
Route ID: 65535
Direction: 3
Supplementary Info: 2
Reserved A: 7
Route Name: 
Reserved B: 172
Waypoint ID: 0
Waypoint Name: 
Lat & Long undefined
Waypoint ID: 1
Waypoint Name: 005
Lat: 39 0.91 (399072996)
Lat: 3 0.09 (30855840)

*/
		root.Clear();

		root["nmea2000"]["pgn"] = 129285;
		root["nmea2000"]["source"] = 7; // BUG BUG Need to get our network address !!
		root["nmea2000"]["destination"] = 255;
		root["nmea2000"]["priority"] = 7;
	
		unsigned short rps = USHRT_MAX;
		root["nmea2000"]["data"][0] = rps & 0xFF;
		root["nmea2000"]["data"][1] = (rps >> 8) & 0xFF;

		int nItems = 2; // We will just specify origin & destination waypoint
		root["nmea2000"]["data"][2] = nItems & 0xFF;
		root["nmea2000"]["data"][3] = (nItems >> 8) & 0xFF;

		unsigned short databaseVersion = USHRT_MAX;
		root["nmea2000"]["data"][4] = databaseVersion & 0xFF;
		root["nmea2000"]["data"][5] = (databaseVersion >> 8) & 0xFF;


		root["nmea2000"]["data"][6] = 0xFF; // navigationData.routeId & 0xFF;
		root["nmea2000"]["data"][7] = 0xFF; // (navigationData.routeId >> 8) & 0xFF;

		unsigned char direction = 3; // I presume forward/reverse, 3 undefined ??
		unsigned char supplementaryInfo = 0xF;
		root["nmea2000"]["data"][8] = ((direction << 5) & 0xE0) |
			((supplementaryInfo << 3) & 0x18) | 0x07;

		// As we need to iterate repeated fields with variable length strings
		// can't use hardcoded indexes into the payload
		int index = 9;
		root["nmea2000"]["data"][index] = 2; // navigationData.routeName.size() + 2; // Length includes length & encodong bytes
		index++;
		root["nmea2000"]["data"][index] = 1; // 1 indicates ASCII Encoding
		index++;
		//for (size_t i = 0; i < navigationData.routeName.size(); i++) {
		//	root["nmea2000"]["data"][index] = navigationData.routeName.at(i);
		//	index++;
		//}

		// Reserved Value
		root["nmea2000"]["data"][index] = 0xFF;
		index++;

		// repeated fields for the two sets of waypoints
		root["nmea2000"]["data"][index] = 0; // navigationData.originId & 0xFF;
		root["nmea2000"]["data"][index + 1] = 0; // (navigationData.originId >> 8) & 0xFF;
		index += 2;

		root["nmea2000"]["data"][index] = 2; // navigationData.originName.size() + 2;
		index++;
		root["nmea2000"]["data"][index] = 1; // 1 indicates SCII encoding
		index++;
		//for (size_t i = 0; i < navigationData.originName.size(); i++) {
		//	root["nmea2000"]["data"][index] = navigationData.originName.at(i);
		//	index++;
		//}

		root["nmea2000"]["data"][index] = 0xFF;
		root["nmea2000"]["data"][index + 1] = 0xFF;
		root["nmea2000"]["data"][index + 2] = 0xFF;
		root["nmea2000"]["data"][index + 3] = 0x7F;

		root["nmea2000"]["data"][index + 4] = 0xFF;
		root["nmea2000"]["data"][index + 5] = 0xFF;
		root["nmea2000"]["data"][index + 6] = 0xFF;
		root["nmea2000"]["data"][index + 7] = 0x7F;

		index += 8;

		root["nmea2000"]["data"][index] = 1; // navigationData.destinationId & 0xFF;
		root["nmea2000"]["data"][index + 1] = 0; // (navigationData.destinationId >> 8) & 0xFF;
		index += 2;

		root["nmea2000"]["data"][index] = navigationData.destinationName.size() + 2;
		index++;
		root["nmea2000"]["data"][index] = 1; // 1 indicates SCII encoding
		index++;
		for (size_t i = 0; i < navigationData.destinationName.size(); i++) {
			root["nmea2000"]["data"][index] = navigationData.destinationName.at(i);
			index++;
		}
		
		root["nmea2000"]["data"][index] = navigationData.destinationLatitude & 0xFF;
		root["nmea2000"]["data"][index + 1] = (navigationData.destinationLatitude >> 8) & 0xFF;
		root["nmea2000"]["data"][index + 2] = (navigationData.destinationLatitude >> 16) & 0xFF;
		root["nmea2000"]["data"][index + 3] = (navigationData.destinationLatitude >> 24) & 0xFF;

		root["nmea2000"]["data"][index + 4] = navigationData.destinationLongitude & 0xFF;
		root["nmea2000"]["data"][index + 5] = (navigationData.destinationLongitude >> 8) & 0xFF;
		root["nmea2000"]["data"][index + 6] = (navigationData.destinationLongitude >> 16) & 0xFF;
		root["nmea2000"]["data"][index + 7] = (navigationData.destinationLongitude >> 24) & 0xFF;

		index += 8;
		root["nmea2000"]["dlc"] = index;

		writer.Write(root, message_body);
		SendPluginMessage(_T("TWOCAN_TRANSMIT_FRAME"), message_body);
	}
	

	// Keep Alive messages are constructed by TwoCan plugin and differ for each model of autopilot
	root.Clear();
	root["autopilot"]["keepalive"] = true;
	writer.Write(root, message_body);
	SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
}

// Handle events from the dialog
// Encode the JSON commands to send to the twocan autopilot device so it can generate the NMEA 2000 messages
void AutopilotPlugin::OnDialogEvent(wxCommandEvent &event) {
	wxString message_body;
	wxJSONValue root;
	wxJSONWriter writer;
	switch (event.GetId()) {
		case AUTOPILOT_STATUS_CHANGED:
			root["autopilot"]["mode"] = std::atoi(event.GetString());
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
			// Most Autopilots require Confirmation when selecting GPS mode, which usually is sending the same message twice
			// BUG BUG Do I send confirmation from here
			if (std::atoi(event.GetString()) == AUTOPILOT_MODE::NAV) {
				if (navigationData.navigationHalted == true) {
					// No Nav Data. // BUG BUG Handle here because EnableGPS doesn't work as expected
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					wxMessageBox("No Route or Waypoint selected");
				}
				else {
					// Send the confirmation
					wxSleep(10);
					SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
				}
			}
			else {
				// Not in Nav mode, so clear the Route Label
				autopilotDialog->SetStatusLabel("No Route");
			}
			break;

		case AUTOPILOT_HEADING_CHANGED:
			if (autopilotMode == AUTOPILOT_MODE::COMPASS) {
				root["autopilot"]["heading"] = std::atoi(event.GetString());
				writer.Write(root, message_body);
				SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
			}
		
			else if (autopilotMode == AUTOPILOT_MODE::WIND) {
				root["autopilot"]["windangle"] = std::atoi(event.GetString());
				writer.Write(root, message_body);
				SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
			}
			else if (autopilotMode == AUTOPILOT_MODE::NAV) {
				// BUG BUG What do we do when we change course when in Nav mode
				// Is this like "dodging", in which case shoud we change mode to COMPASS
			}
			break;
			
		// BUG BUG Not used
		case AUTOPILOT_WAYPOINT_CHANGED: 
			root["autopilot"]["waypoint"] = event.GetString();
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
			break;

	}

}