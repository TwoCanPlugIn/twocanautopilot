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
AutopilotPlugin::AutopilotPlugin(void *ppimgr) : opencpn_plugin_117(ppimgr),  wxEvtHandler() {
	
	// Load the plugin icon
	initialize_images();

	// Initialize Advanced User Interface Manager (AUI)
	auiManager = GetFrameAuiManager();

	// Structure used to aggregate data necessary for configuring the autopilot to follow a route
	navigationData = {};
	navigationData.navigationHalted = TRUE;


	// Start a one second timer to send keep alive messages to the autopilot
	// When in GPS mode, send navigation and cross track error messages
	oneSecondTimer = new wxTimer();
	oneSecondTimer->Bind(wxEVT_TIMER, &AutopilotPlugin::OnTimerElapsed, this);
	oneSecondTimer->Start(1000, wxTIMER_CONTINUOUS);
}

// Destructor
AutopilotPlugin::~AutopilotPlugin(void) {

	oneSecondTimer->Stop();
	oneSecondTimer->Unbind(wxEVT_TIMER, &AutopilotPlugin::OnTimerElapsed, this);
	delete oneSecondTimer;

	delete _img_autopilot;
}

int AutopilotPlugin::Init(void) {
	// Maintain a reference to the OpenCPN window to use as the parent
	parentWindow = GetOCPNCanvasWindow();

	// Maintain a reference to the OpenCPN configuration object 
	configSettings = GetOCPNConfigObject();

	// Load Configuration Settings
	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/TwoCan"));
		autopilotModel = (AUTOPILOT_MODEL)configSettings->ReadLong(_T("Autopilot"), AUTOPILOT_MODEL::NONE);

		// BUG BUG If no Autopilot has been selected, is this sufficient to "disable" the plugin?
		if (autopilotModel == AUTOPILOT_MODEL::NONE) {
			wxLogMessage(_T("TwoCan Autopilot, Invalid or missing autopilot configuration"));
		}

		configSettings->SetPath(_T("/PlugIns/TwoCanAutopilot"));
		configSettings->Read(_T("Visible"), &autopilotDialogVisible, FALSE);

		// In case the user has changed the default autopilot NMEA 0183 talker ID from EC 
		configSettings->SetPath(_T("/Settings"));
		configSettings->Read(_T("TalkerIdText"), &talkerId, "EC");
		talkerId.Prepend("$");
	}
	else {
		autopilotDialogVisible = FALSE;
	}


	// Load toolbar icons
	wxString shareLocn = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator() 
		+ _T("images") + wxFileName::GetPathSeparator();
	
	wxString normalIcon = shareLocn + _T("autopilot-normal.svg");
	wxString toggledIcon = shareLocn + _T("autopilot-toggled.svg");
	wxString rolloverIcon = shareLocn + _T("autopilot-rollover.svg");

	// Insert the toolbar icons
	autopilotToolbar = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK, _("TwoCan Autopilot"), _T(""), NULL, -1, 0, this);

	// If no valid autopilot is configured in TwoCan plugin 
	SetToolbarToolViz(autopilotToolbar, (autopilotModel != AUTOPILOT_MODEL::NONE));

	// Instantiate the autopilot dialog
	autopilotDialog = new  AutopilotDialog(parentWindow, this);

	// Only enable GPS Mode if navigation is active
	autopilotDialog->EnableGPSMode(FALSE);

	// No alarms to display (yet...)
	autopilotDialog->SetAlarmLabel(wxEmptyString);

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
	paneInfo.Name(_T(PLUGIN_COMMON_NAME));
	paneInfo.Caption(_T(PLUGIN_SHORT_DESCRIPTION));
	paneInfo.CloseButton(TRUE);
	paneInfo.Float();
	paneInfo.Dockable(FALSE);
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
		configSettings->SetPath(_T("/PlugIns/TwoCanAutopilot"));
		configSettings->Write(_T("Visible"), autopilotDialogVisible);
	}
	return TRUE;
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
	return _img_autopilot;
}

// We install one toolbar item
int AutopilotPlugin::GetToolbarToolCount(void) {
 return 1;
}

int AutopilotPlugin::GetToolbarItemId() { 
	return autopilotToolbar; 
}

void AutopilotPlugin::SetDefaults(void) {
	// Is called when the plugin is enabled from the Plugin Manager dialog
	// Provides an opportunity to configure default values ??
}

// UpdateAUI Status is invoked by OpenCPN when the saved AUI perspective is loaded
void AutopilotPlugin::UpdateAuiStatus(void) {
	auiManager->GetPane(_T(PLUGIN_COMMON_NAME)).Show(autopilotDialogVisible);
	auiManager->Update();
	SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
}

// Toggle the display of dialog as appropriate when the toolbar button is pressed
void AutopilotPlugin::OnToolbarToolCallback(int id) {
	if ((id == autopilotToolbar) && (autopilotModel != AUTOPILOT_MODEL::NONE)) {
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
		autopilotDialogVisible = FALSE;
		SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
	}
	else {
		event.Skip();
	}
}

// Receive NMEA 183 Sentences from OpenCPN
// We use these when in Wind or GPS mode
// BUG BUG For OpenCPN 5.8.x, contemplate using the NMEA 183 Listeners
// BUG BUG. If no NMEA 0183 connection, may need to need to support SignalK or NMEA 2000. Aaarrggghhh!
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
		// BUG BUG Perhaps uneccesary as we use PositionFixEx to update 
		// UI when we are in Heading Hold/Compass mode
		else if (autopilotMode == AUTOPILOT_MODE::COMPASS) {
			if (nmea183.LastSentenceIDReceived == _T("HDG")) {
#if defined (__WXMSW__)
				OutputDebugString(sentence);
#endif
				//if (nmea183.Parse()) {
				//	autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f",
				//		nmea183.Hdg.MagneticSensorHeadingDegrees));

				//}
			}
		}

		else if (autopilotMode == AUTOPILOT_MODE::NAV) {
			if (sentence.StartsWith(talkerId)) {
#if defined (__WXMSW__)
				OutputDebugString(sentence);
#endif
				// If we have an active route or navigating to a waypoint
				// Need to send PGN's 129283 & 129284 to the autopilot
				// These PGN's are constructed from data present in RMB, XTE and/or APB sentences
				if (navigationData.navigationHalted == FALSE) {
					if (nmea183.LastSentenceIDReceived == _T("RMB")) {
						if (nmea183.Parse()) {
							if (nmea183.Rmb.IsDataValid == NTrue) {

								// BUG BUG Could populate from the OCPN Plugn Messages and Lookup Waypoint Function
								navigationData.destinationLatitude = nmea183.Rmb.DestinationPosition.Latitude.Latitude;
								if (nmea183.Rmb.DestinationPosition.Latitude.Northing == NORTHSOUTH::South) {
									navigationData.destinationLatitude = -navigationData.destinationLatitude;
								}

								navigationData.destinationLongitude = nmea183.Rmb.DestinationPosition.Longitude.Longitude;
								if (nmea183.Rmb.DestinationPosition.Longitude.Easting == EASTWEST::West) {
									navigationData.destinationLongitude = -navigationData.destinationLongitude;
								}

								navigationData.xteMode = GetFAAMode(nmea183.Rmb.FAAModeIndicator);

								// BUG BUG is the sign of cross track error correct ??
								navigationData.crossTrackError = nmea183.Rmb.CrossTrackError;

								// BUG BUG just to confirm sign of XTE
#if defined (__WXMSW__)
								OutputDebugString(wxString::Format("XTE: %d\n", navigationData.crossTrackError));

#endif
								if (nmea183.Rmb.DirectionToSteer == LEFTRIGHT::Left) {
									navigationData.crossTrackError = -navigationData.crossTrackError;
								}

								navigationData.waypointClosingVelocity = nmea183.Rmb.DestinationClosingVelocityKnots;

								navigationData.distanceToWaypoint = nmea183.Rmb.RangeToDestinationNauticalMiles;

							}

						}
					}
					if (nmea183.LastSentenceIDReceived == _T("APB")) {
						if (nmea183.Parse()) {
							if (nmea183.Apb.BearingPresentPositionToDestinationUnits == "True") {
								navigationData.bearingReference = TRUE;
							}
							else {
								navigationData.bearingReference = FALSE;
							}
							navigationData.currentBearing = nmea183.Apb.BearingPresentPositionToDestination;
							navigationData.originalBearing = nmea183.Apb.BearingOriginToDestination;
							if (nmea183.Apb.IsArrivalCircleEntered == NTrue) {
								navigationData.arrivalCircleEntered = TRUE;
							}
							else {
								navigationData.arrivalCircleEntered = FALSE;
							}
							if (nmea183.Apb.IsPerpendicular == NTrue) {
								navigationData.perpendicularCrossed = TRUE;
							}
							else {
								navigationData.perpendicularCrossed = FALSE;
							}
						}
					}
				}
			}
		}
	}
}

// Receive OpenCPN Plugin Messages.
// OpenCPN sends messages to indicate if a route or waypoint has been activated.
// TwoCan also sends status messages back to us for display in the UI
void AutopilotPlugin::SetPluginMessage(wxString &message_id, wxString &message_body) {

	// Brief description of the JSON schema used by TwoCan Autopilot
	// Some are send & receive, some are receive only and some send only

	// root["autopilot"]["mode"] 0 standby | 1 heading (compass)  | 2 nav (gps) | 3 wind | 4 no drift (not implemented)
	// root["autopilot"]["heading"] in degrees
	// root["autopilot"]["windangle"] in +/- degrees - port, + starboard
	// root["autopilot"]["rudderangle"] in +/- degrees
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
			// The Autopilot may be controlled from another controller
			if (root["autopilot"].HasMember("mode")) {
				autopilotDialog->SetMode(static_cast<AUTOPILOT_MODE>(root["autopilot"]["mode"].AsInt()));
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

			// BUG BUG Does TwoCan parse any alarms ??
			if (root["autopilot"].HasMember("alarm")) {
				wxString alarm = root["autopilot"]["alarm"].AsString();
				autopilotDialog->SetAlarmLabel(alarm);
			}

			// BUG BUG Need a Linear Meter to display rudder angle
			if (root["autopilot"].HasMember("rudderangle")) {
				int rudderAngle = root["autopilot"]["rudderangle"].AsInt();
				//autopilotDialog->SetHeadingLabel(wxString::Format("Rudder Angle: %d", rudderAngle);
			}
		}

		else if (message_id == _T("OCPN_RTE_ACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif
			navigationData.navigationHalted = FALSE;
			navigationData.routeId = strtoul(root["GUID"].AsString().Mid(0, 6), NULL, 16);
			navigationData.routeName = LookupRouteName(root["GUID"].AsString());
			if (navigationData.routeName.size() == 0) {
				navigationData.routeName = "Unamed";
			}

			// BUG BUG Need to look up start and ending waypoint names
			
			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel(wxString::Format("Route: %s", navigationData.routeName));
				autopilotDialog->EnableGPSMode(TRUE);
			}
		}

		else if (message_id == _T("OCPN_RTE_DEACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif

			navigationData.routeName.clear();
			navigationData.navigationHalted = TRUE;

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Route: Deactivated");
				autopilotDialog->EnableGPSMode(FALSE);
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					root.Clear();
					root["autopilot"]["mode"] = AUTOPILOT_MODE::STANDBY;
					writer.Write(root, message_body);
					SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
				}
			}
		}

		else if (message_id == _T("OCPN_RTE_ENDED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif

			navigationData.routeName.clear();
			navigationData.navigationHalted = TRUE;
			// BUG BUG Confirm if following are reflected in APB or RMB sentence
			navigationData.arrivalCircleEntered = TRUE;
			navigationData.perpendicularCrossed = TRUE;
			

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Route: Complete");
				autopilotDialog->EnableGPSMode(FALSE);
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					root.Clear();
					root["autopilot"]["mode"] = AUTOPILOT_MODE::STANDBY;
					writer.Write(root, message_body);
					SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
				}
			}
		}

		else if (message_id == _T("OCPN_WPT_ACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif

			navigationData.navigationHalted = FALSE;
			navigationData.originId = 0;
			navigationData.originName = "Start"; 
			// BUG BUG Can we determine if there is a starting position name

			navigationData.destinationName = LookupWaypoint(root["GUID"].AsString()).m_MarkName;
			if (navigationData.destinationName.size() == 0) {
				navigationData.destinationName = "End";
			}
			navigationData.destinationId = strtoul(root["GUID"].AsString().Mid(0, 6), NULL, 16);

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel(wxString::Format("Waypoint: %s", navigationData.destinationName));
				autopilotDialog->EnableGPSMode(TRUE);
			}
		}

		else if (message_id == _T("OCPN_WPT_DEACTIVATED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif
			navigationData.navigationHalted = TRUE;
			navigationData.destinationName.clear();

			if (autopilotDialog != nullptr) {
				autopilotDialog->SetStatusLabel("Waypoint: Deactivated");
				autopilotDialog->EnableGPSMode(FALSE);
				if (autopilotMode == AUTOPILOT_MODE::NAV) {
					autopilotDialog->SetMode(AUTOPILOT_MODE::STANDBY);
					root.Clear();
					root["autopilot"]["mode"] = AUTOPILOT_MODE::STANDBY;
					writer.Write(root, message_body);
					SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
				}
			}
		}

		else if (message_id == _T("OCPN_WPT_ARRIVED")) {
#if defined (__WXMSW__)
			OutputDebugString(message_id);
			OutputDebugString(message_body);
#endif 
			if (root.HasMember("GUID_Next_WP")) {

				// BUG BUG Should there be a confirm dialog ?

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
					autopilotDialog->EnableGPSMode(TRUE);
				}
			}
			else {
				// No next waypoint, navigation has ended
				navigationData.navigationHalted = TRUE;

				if (autopilotDialog != nullptr) {
					// Wouldn't this just be the destinationName ??
					autopilotDialog->SetStatusLabel(wxString::Format("Arrived: %s",
					LookupWaypoint(root["GUID_WP_arrived"].AsString()).m_MarkName));
					autopilotDialog->EnableGPSMode(FALSE);
				}
			}
		}
	}
}

// Send XTE and Bearing data to TwoCan plugin to generate PGN 127283 & 127284 messages.
// BUG BUG Not used as data is derived from NMEA183 APB and RMB sentences
// In anycase not enough info is avalable in Plugin_Active_Leg_Info
void AutopilotPlugin::SetActiveLegInfo(Plugin_Active_Leg_Info &pInfo) {
}

// Update our heading if not in Wind Mode
// Alternative is to parse HDG sentence in SetNmeaSentence
void AutopilotPlugin::SetPositionFixEx(PlugIn_Position_Fix_Ex &pfix) {
	if (autopilotDialog != nullptr) {
		if (autopilotMode != AUTOPILOT_MODE::WIND) {
			if (isnan(pfix.Hdm)) {
				autopilotDialog->SetHeadingLabel("---");
			}
			else {
				autopilotDialog->SetHeadingLabel(wxString::Format("Heading: %.1f", pfix.Hdm));
			}
		}
	}
}

// Used to convert FAA Mode from NMEA 183 (char) to NMEA 2000 (int)
int AutopilotPlugin::GetFAAMode(wxString mode) {
	// Order of array matches NMEA 2000 mode values
	wxString FAAModes[] = { "A","D","E","M","S" };
	for (size_t i = 0; i < 5; i++) {
		if (mode == FAAModes[i]) {
			return i;
		}
	}
	// NMEA 2000 FAA Mode, unavailable = 0x0F
	return 0x0F;
}

// Used to fill fields to generate PGN 129285
PlugIn_Waypoint AutopilotPlugin::LookupWaypoint(wxString guid) {
	PlugIn_Waypoint waypoint;
	GetSingleWaypoint(guid, &waypoint);
	return waypoint;
}

// Used to fill fields to generate PGN 129285
wxString AutopilotPlugin::LookupRouteName(wxString guid) {
	std::unique_ptr<PlugIn_Route> activeRoute;
	activeRoute = GetRoute_Plugin(guid);
	return activeRoute->m_NameString;
}

// One Second Timer used to send XTE, Navigation, Route and Keep Alive messages
void AutopilotPlugin::OnTimerElapsed(wxEvent &event) {
	
	if (oneSecondTimer->IsRunning()) {

		wxString message_body;
		wxJSONValue root;
		wxJSONWriter writer;

		// Send Keep Alive messages
		if (autopilotMode != AUTOPILOT_MODE::STANDBY) {
			root.Clear();
			root["autopilot"]["keepalive"] = TRUE;
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
		}

		// When in GPS mode, send PGN's 129283,129284
		if (autopilotMode == AUTOPILOT_MODE::NAV) {

			// PGN 129283, NMEA Cross Track Error
			root.Clear();
			root["autopilot"]["pgn129283"]["crossTrackError"] = 100 * (navigationData.crossTrackError / CONVERT_METRES_NAUTICAL_MILES);;
			root["autopilot"]["pgn129283"]["halted"] = navigationData.navigationHalted;
			root["autopilot"]["pgn129283"]["mode"] = navigationData.xteMode;
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);

	//		// The following is using the TWOCAN_TRANSMIT_FRAME capability
	//		root.Clear();
	//		root["nmea2000"]["pgn"] = 129283;
	//		root["nmea2000"]["source"] = 7; // BUG BUG Network Address filled in by TwoCan
	//		root["nmea2000"]["destination"] = 255;
	//		root["nmea2000"]["priority"] = 7;
	//		root["nmea2000"]["dlc"] = 8;

	//		root["nmea2000"]["data"][0] = 0xFF; // Sequence Id
	//		//xte Mode set to 0 = Autonomous Mode, 0x30 is reserved
	//		root["nmea2000"]["data"][1] = (0 & 0x0F) | 0x30 | ((navigationData.navigationHalted << 6) & 0xC0);

	//		int crosstrackError = 100 * (navigationData.crossTrackError / CONVERT_METRES_NAUTICAL_MILES);

	//		root["nmea2000"]["data"][2] = navigationData.crossTrackError & 0xFF;
	//		root["nmea2000"]["data"][3] = (navigationData.crossTrackError >> 8) & 0xFF;
	//		root["nmea2000"]["data"][4] = (navigationData.crossTrackError >> 16) & 0xFF;
	//		root["nmea2000"]["data"][5] = (navigationData.crossTrackError >> 24) & 0xFF;
	//		root["nmea2000"]["data"][6] = 0xFF;
	//		root["nmea2000"]["data"][7] = 0xFF;

	//		writer.Write(root, message_body);
	//		SendPluginMessage(_T("TWOCAN_TRANSMIT_FRAME"), message_body);

			// PGN 129284, NMEA Navigation Data
			// Note conversion to SI units
			root.Clear();
			root["autopilot"]["pgn129284"]["range"] = (navigationData.distanceToWaypoint / CONVERT_METRES_NAUTICAL_MILES) * 100;
			root["autopilot"]["pgn129284"]["perpendicular"] = navigationData.perpendicularCrossed; 
			root["autopilot"]["pgn129284"]["arrivalcircle"] = navigationData.arrivalCircleEntered;
			unsigned short daysSinceEpoch;
			unsigned int secondsSinceMidnight;
			navigationData.GetETA(&daysSinceEpoch, &secondsSinceMidnight);
			root["autopilot"]["pgn129284"]["days"] = daysSinceEpoch;
			root["autopilot"]["pgn129284"]["seconds"] = secondsSinceMidnight;		
			root["autopilot"]["pgn129284"]["latitude"] = navigationData.destinationLatitude * 1e7;
			root["autopilot"]["pgn129284"]["longitude"] = navigationData.destinationLongitude * 1e7;
			root["autopilot"]["pgn129284"]["start"] = 0xFF; // DestinationId & OriginID unnecessary ??
			root["autopilot"]["pgn129284"]["end"] = 0xFF; 
			root["autopilot"]["pgn129284"]["origin"] = DEGREES_TO_RADIANS(navigationData.originalBearing) * 1e4;
			root["autopilot"]["pgn129284"]["current"] = DEGREES_TO_RADIANS(navigationData.currentBearing) * 1e4;
			root["autopilot"]["pgn129284"]["velocity"] = (navigationData.waypointClosingVelocity / CONVERT_MS_KNOTS) * 100;
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);

			// The following is using the TWOCAN_TRANSMIT_FRAME capability
	//		root["nmea2000"]["pgn"] = 129284;
	//		root["nmea2000"]["source"] = 7; // BUG BUG Need to get our network address !!
	//		root["nmea2000"]["destination"] = 255;
	//		root["nmea2000"]["priority"] = 7;
	//		root["nmea2000"]["dlc"] = 34;

	//		root["nmea2000"]["data"][0] = 0xFF; // sequence Id;

	//		unsigned int distanceToWaypoint = 100 * (navigationData.distanceToWaypoint / CONVERT_METRES_NAUTICAL_MILES);
	//		root["nmea2000"]["data"][1] = distanceToWaypoint & 0xFF;
	//		root["nmea2000"]["data"][2] = (distanceToWaypoint >> 8) & 0xFF;
	//		root["nmea2000"]["data"][3] = (distanceToWaypoint >> 16) & 0xFF;
	//		root["nmea2000"]["data"][4] = (distanceToWaypoint >> 24) & 0xFF;

	//		root["nmea2000"]["data"][5] = (1 << 6) & 0xC0;
	//		root["nmea2000"]["data"][5] = root["nmea2000"]["data"][5].AsInt() | ((navigationData.perpendicularCrossed << 4) & 0x30);
	//		root["nmea2000"]["data"][5] = root["nmea2000"]["data"][5].AsInt() | ((navigationData.arrivalCircleEntered << 2) & 0xC);
	//		root["nmea2000"]["data"][5] = root["nmea2000"]["data"][5].AsInt() | (0 & 0x03);
	//		unsigned short daysSinceEpoch;
	//		unsigned int secondsSinceMidnight;
	//		navigationData.GetETA(&daysSinceEpoch, &secondsSinceMidnight);
	//		root["nmea2000"]["data"][6] = secondsSinceMidnight & 0xFF;
	//		root["nmea2000"]["data"][7] = (secondsSinceMidnight >> 8) & 0xFF;
	//		root["nmea2000"]["data"][8] = (secondsSinceMidnight >> 16) & 0xFF;
	//		root["nmea2000"]["data"][9] = (secondsSinceMidnight >> 24) & 0xFF;

	//		root["nmea2000"]["data"][10] = daysSinceEpoch & 0xFF;
	//		root["nmea2000"]["data"][11] = (daysSinceEpoch >> 8) & 0xFF;

	//		root["nmea2000"]["data"][12] = navigationData.originalBearing & 0xFF;
	//		root["nmea2000"]["data"][13] = (navigationData.originalBearing >> 8) & 0xFF;

	//		root["nmea2000"]["data"][14] = navigationData.currentBearing & 0xFF;
	//		root["nmea2000"]["data"][15] = (navigationData.currentBearing >> 8) & 0xFF;

	//		//B&G Just uses 0 and 1 for Origin and Destination Id
	//		root["nmea2000"]["data"][16] = 0; // navigationData.originId & 0xFF;
	//		root["nmea2000"]["data"][17] = 0; // (navigationData.originId >> 8) & 0xFF;
	//		root["nmea2000"]["data"][18] = 0; // (navigationData.originId >> 16) & 0xFF;
	//		root["nmea2000"]["data"][19] = 0; // (navigationData.originId >> 24) & 0xFF;

	//		root["nmea2000"]["data"][20] = 1;// navigationData.destinationId & 0xFF;
	//		root["nmea2000"]["data"][21] = 0;// (navigationData.destinationId >> 8) & 0xFF;
	//		root["nmea2000"]["data"][22] = 0;// (navigationData.destinationId >> 16) & 0xFF;
	//		root["nmea2000"]["data"][23] = 0;// (navigationData.destinationId >> 24) & 0xFF;

	//		root["nmea2000"]["data"][24] = navigationData.destinationLatitude & 0xFF;
	//		root["nmea2000"]["data"][25] = (navigationData.destinationLatitude >> 8) & 0xFF;
	//		root["nmea2000"]["data"][26] = (navigationData.destinationLatitude >> 16) & 0xFF;
	//		root["nmea2000"]["data"][27] = (navigationData.destinationLatitude >> 24) & 0xFF;

	//		root["nmea2000"]["data"][28] = navigationData.destinationLongitude & 0xFF;
	//		root["nmea2000"]["data"][29] = (navigationData.destinationLongitude >> 8) & 0xFF;
	//		root["nmea2000"]["data"][30] = (navigationData.destinationLongitude >> 16) & 0xFF;
	//		root["nmea2000"]["data"][31] = (navigationData.destinationLongitude >> 24) & 0xFF;

	//	if (navigationData.waypointClosingVelocity > 0) {
	//		short speed = /100 * (navigationData.waypointClosingVelocity / CONVERT_MS_KNOTS);
	//		root["nmea2000"]["data"][32] = speed & 0xFF;
	//		root["nmea2000"]["data"][33] = (speed >> 8) & 0xFF;
	//	}
	//	else {
	//		root["nmea2000"]["data"][32] = 0xFF;
	//		root["nmea2000"]["data"][33] = 0xFF;
	//	}
	//		 
	//		writer.Write(root, message_body);
	//		SendPluginMessage(_T("TWOCAN_TRANSMIT_FRAME"), message_body);

	//

	//BUG BUG PGN 129285, NMEA Route/Waypoint Information
	// Not used
	//*
	//Route Name Length: 3
	//RPS: 65535
	//Items: 2
	//Database Version: 65535
	//Route ID: 65535
	//Direction: 3
	//Supplementary Info: 2
	//Reserved A: 7
	//Route Name:
	//Reserved B: 172
	//Waypoint ID: 0
	//Waypoint Name:
	//Lat & Long undefined
	//Waypoint ID: 1
	//Waypoint Name: 005
	//Lat: 39 0.91 (399072996)
	//Lat: 3 0.09 (30855840)

	//
	//
	//root.Clear();

	//root["nmea2000"]["pgn"] = 129285;
	//root["nmea2000"]["source"] = 7; // BUG BUG Need to get our network address !!
	//root["nmea2000"]["destination"] = 255;
	//root["nmea2000"]["priority"] = 7;

	//unsigned short rps = USHRT_MAX;
	//root["nmea2000"]["data"][0] = rps & 0xFF;
	//root["nmea2000"]["data"][1] = (rps >> 8) & 0xFF;

	//int nItems = 2; // We will just specify origin & destination waypoint
	//root["nmea2000"]["data"][2] = nItems & 0xFF;
	//root["nmea2000"]["data"][3] = (nItems >> 8) & 0xFF;

	//unsigned short databaseVersion = USHRT_MAX;
	//root["nmea2000"]["data"][4] = databaseVersion & 0xFF;
	//root["nmea2000"]["data"][5] = (databaseVersion >> 8) & 0xFF;


	//root["nmea2000"]["data"][6] = 0xFF; // navigationData.routeId & 0xFF;
	//root["nmea2000"]["data"][7] = 0xFF; // (navigationData.routeId >> 8) & 0xFF;

	//unsigned char direction = 3; // I presume forward/reverse, 3 undefined ??
	//unsigned char supplementaryInfo = 0xF;
	//root["nmea2000"]["data"][8] = ((direction << 5) & 0xE0) |
	//	((supplementaryInfo << 3) & 0x18) | 0x07;

	// As we need to iterate repeated fields with variable length strings
	// can't use hardcoded indexes into the payload
	//int index = 9;
	//root["nmea2000"]["data"][index] = 2; // navigationData.routeName.size() + 2; // Length includes length & encodong bytes
	//index++;
	//root["nmea2000"]["data"][index] = 1; // 1 indicates ASCII Encoding
	//index++;
	//for (size_t i = 0; i < navigationData.routeName.size(); i++) {
	//	root["nmea2000"]["data"][index] = navigationData.routeName.at(i);
	//	index++;
	//}

	// Reserved Value
	//root["nmea2000"]["data"][index] = 0xFF;
	//index++;

	// repeated fields for the two sets of waypoints
	//root["nmea2000"]["data"][index] = 0; // navigationData.originId & 0xFF;
	//root["nmea2000"]["data"][index + 1] = 0; // (navigationData.originId >> 8) & 0xFF;
	//index += 2;

	//root["nmea2000"]["data"][index] = 2; // navigationData.originName.size() + 2;
	//index++;
	//root["nmea2000"]["data"][index] = 1; // 1 indicates SCII encoding
	//index++;
	//for (size_t i = 0; i < navigationData.originName.size(); i++) {
	//	root["nmea2000"]["data"][index] = navigationData.originName.at(i);
	//	index++;
	//}

	//root["nmea2000"]["data"][index] = 0xFF;
	//root["nmea2000"]["data"][index + 1] = 0xFF;
	//root["nmea2000"]["data"][index + 2] = 0xFF;
	//root["nmea2000"]["data"][index + 3] = 0x7F;

	//root["nmea2000"]["data"][index + 4] = 0xFF;
	//root["nmea2000"]["data"][index + 5] = 0xFF;
	//root["nmea2000"]["data"][index + 6] = 0xFF;
	//root["nmea2000"]["data"][index + 7] = 0x7F;

	//index += 8;

	//root["nmea2000"]["data"][index] = 1; // navigationData.destinationId & 0xFF;
	//root["nmea2000"]["data"][index + 1] = 0; // (navigationData.destinationId >> 8) & 0xFF;
	//index += 2;

	//root["nmea2000"]["data"][index] = navigationData.destinationName.size() + 2;
	//index++;
	//root["nmea2000"]["data"][index] = 1; // 1 indicates SCII encoding
	//index++;
	//for (size_t i = 0; i < navigationData.destinationName.size(); i++) {
	//	root["nmea2000"]["data"][index] = navigationData.destinationName.at(i);
	//	index++;
	//}

	//root["nmea2000"]["data"][index] = navigationData.destinationLatitude & 0xFF;
	//root["nmea2000"]["data"][index + 1] = (navigationData.destinationLatitude >> 8) & 0xFF;
	//root["nmea2000"]["data"][index + 2] = (navigationData.destinationLatitude >> 16) & 0xFF;
	//root["nmea2000"]["data"][index + 3] = (navigationData.destinationLatitude >> 24) & 0xFF;

	//root["nmea2000"]["data"][index + 4] = navigationData.destinationLongitude & 0xFF;
	//root["nmea2000"]["data"][index + 5] = (navigationData.destinationLongitude >> 8) & 0xFF;
	//root["nmea2000"]["data"][index + 6] = (navigationData.destinationLongitude >> 16) & 0xFF;
	//root["nmea2000"]["data"][index + 7] = (navigationData.destinationLongitude >> 24) & 0xFF;

	//index += 8;
	//root["nmea2000"]["dlc"] = index;

	//writer.Write(root, message_body);
	//SendPluginMessage(_T("TWOCAN_TRANSMIT_FRAME"), message_body);

	//
		}

	}
}

// Refer to Douwe Fokkema's Raymarine Autopilot Plugin for following
// If in GPS Mode, navigating to a waypoint, we can use this as an
// alternative algorithm to steer, instead of using the autopilot's 
// tracking mode algorithm

//void AutopilotPlugin::Compute() {
//	double dist;
//	double XTE_for_correction;
//
//	if (isnan(navigationData.currentBearing)) {
//		return;
//	}
//	if (isnan(navigationData.crossTrackError) || navigationData.crossTrackError == 100000.) {
//		return;
//	}
//	if (autopilotMode != AUTOPILOT_MODE::NAV) {
//		return;
//	}
//	if (navigationData.navigationHalted) {
//		return;
//	}
//	dist = 50; // in meters
//	double dist_nm = dist / 1852.;
//
//	// integration of XTE, but prevent increase of m_XTE_I when XTE is large
//	if (navigationData.crossTrackError > -0.25 * dist_nm && navigationData.crossTrackError < 0.25 * dist_nm) {
//		m_XTE_I += navigationData.crossTrackError;
//	}
//	else if (navigationData.crossTrackError > -0.5 * dist_nm && navigationData.crossTrackError < 0.5 * dist_nm) {
//		m_XTE_I += 0.5 * navigationData.crossTrackError;
//	}
//	else if (navigationData.crossTrackError > -dist_nm && navigationData.crossTrackError < dist_nm) {
//		m_XTE_I += 0.2 * navigationData.crossTrackError;
//	}
//	else {
//	}; // do nothing for now
//
//	m_XTE_D = navigationData.crossTrackError - m_XTE_P; // difference
//	m_XTE_P = navigationData.crossTrackError; // proportional used as previous xte next timw
//
//	if (m_XTE_I > 0.5 * dist_nm / I_FACTOR) { // in NM
//		m_XTE_I = 0.5 * dist_nm / I_FACTOR;
//	}
//	if (m_XTE_I < -0.5 * dist_nm / I_FACTOR) { // in NM
//		m_XTE_I = -0.5 * dist_nm / I_FACTOR;
//	}
//
//	XTE_for_correction = navigationData.crossTrackError + I_FACTOR * navigationData.crossTrackError + D_FACTOR * m_XTE_D;
//
//	wxLogMessage(wxT(" XTE_for_correction=%f, 5 * m_XTE=%f,  I_FACTOR *    m_XTE_I=%f, D_FACTOR * m_XTE_D=%f"),
//		XTE_for_correction, 5 * navigationData.crossTrackError, I_FACTOR * m_XTE_I, D_FACTOR *
//		m_XTE_D);
//
//	double gamma,
//		new_bearing; // angle for correction of heading relative to BTW
//	if (dist > 1.) {
//		gamma = atan(XTE_for_correction * 1852. / dist) / (2. * 3.1416) * 360.;
//	}
//	double max_angle = prefs.max_angle;
//	// wxLogMessage(wxT("AutoTrackRaymarine initial gamma=%f, btw=%f,
//	// dist=%f, max_angle= %f, XTE_for_correction=%f"), gamma, m_BTW, dist,
//	// max_angle, XTE_for_correction);
//	new_bearing = navigationData.currentBearing + gamma; // bearing of next wp
//
//	if (gamma > max_angle) {
//		new_bearing = navigationData.currentBearing + max_angle;
//	}
//	else if (gamma < -max_angle) {
//		new_bearing = navigationData.currentBearing - max_angle;
//	}
//	// don't turn too fast....
//
//	if (!m_heading_set) { // after reset accept any turn
//		m_current_bearing = new_bearing;
//		m_heading_set = true;
//	}
//	else {
//		while (new_bearing >= 360.)
//			new_bearing -= 360.;
//		while (new_bearing < 0.)
//			new_bearing += 360.;
//		double turnrate = TURNRATE;
//
//		// turn left or right?
//		double turn = new_bearing - m_current_bearing;
//
//		if (turn < -180.)
//			turn += 360;
//		if (turn > 80. || turn < -80.)
//			turnrate = 2 * TURNRATE;
//		if (turn < -turnrate || (turn > 180. && turn < 360 - turnrate)) {
//			// turn left
//			m_current_bearing -= turnrate;
//		}
//		else if (turn > turnrate && turn <= 180.) {
//			// turn right
//			m_current_bearing += turnrate;
//		}
//		else {
//			// go almost straight, correction < TURNRATE
//			m_current_bearing = new_bearing;
//		}
//	}
//	while (m_current_bearing >= 360.)
//		m_current_bearing -= 360.;
//	while (m_current_bearing < 0.)
//		m_current_bearing += 360.;
//	SetPilotHeading(
//		m_current_bearing - m_var); // the commands used expect magnetic heading
//	m_pilot_heading = m_currentbearing; // This should not be needed, pilot heading
//							 }
//}

// Handle events from the dialog
// Encode the JSON commands to send to the TwoCan Plugin so it can generate the NMEA 2000 messages
void AutopilotPlugin::OnDialogEvent(wxCommandEvent &event) {
	wxString message_body;
	wxJSONValue root;
	wxJSONWriter writer;
	switch (event.GetId()) {
	case AUTOPILOT_MODE_CHANGED:
		root["autopilot"]["mode"] = event.GetInt();
		writer.Write(root, message_body);
		SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
		// Most Autopilots require Confirmation when selecting GPS mode, which usually 
		// requires sending the same message twice
		if (event.GetInt() == AUTOPILOT_MODE::NAV) {
			// Send the confirmation
			wxSleep(10);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
		}
		break;

	case AUTOPILOT_HEADING_CHANGED:
		if (autopilotMode == AUTOPILOT_MODE::COMPASS) {
			root["autopilot"]["heading"] = event.GetInt();
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
		}
		
		else if (autopilotMode == AUTOPILOT_MODE::WIND) {
			root["autopilot"]["windangle"] = event.GetInt();
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_REQUEST"), message_body);
		}
		else if (autopilotMode == AUTOPILOT_MODE::NAV) {
			// BUG BUG What do we do when we change course when in Nav mode
			// Is this like "dodging", in which case should we change mode to COMPASS
		}
		break;
	}
}