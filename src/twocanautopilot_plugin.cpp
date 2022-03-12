// Copyright(C) 2021 by Steven Adler
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
// Date: 10/01/2022
// Version History:
// 1.0 Initial Release
//

#include "twocanautopilot_plugin.h"
#include "twocanautopilot_icon.h"

// BUG BUG REMOVE
#include <wx/socket.h>

// Some globals accessed by both the plugin and the dialog

// BUG BUG REMOVE 
// Debug spew via UDP
wxDatagramSocket *debugSocket;
wxIPV4address addrLocal;
wxIPV4address addrPeer;

// The class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
	return new AutopilotPlugin(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
	delete p;
}

// Constructor
AutopilotPlugin::AutopilotPlugin(void *ppimgr) : opencpn_plugin_117(ppimgr), wxEvtHandler() {
	
	// Load the plugin icon
	// refer to twocanautopilot_icons.cpp
	initialize_images();

	// Initialize Advanced User Interface Manager (AUI)
	auiManager = GetFrameAuiManager();
}

// Destructor
AutopilotPlugin::~AutopilotPlugin(void) {
	delete autopilotLogo;
}

int AutopilotPlugin::Init(void) {
	// Maintain a reference to the OpenCPN window to use as the parent
	parentWindow = GetOCPNCanvasWindow();

	// Maintain a reference to the OpenCPN configuration object 
	configSettings = GetOCPNConfigObject();

	// Load Configuration Setings
	if (configSettings) {
		configSettings->SetPath(_T("/PlugIns/TwoCanAutopilot"));
		configSettings->Read(_T("Model"), &autopilotModel, "None");
		}
	else {
		autopilotModel = "None";
	}

	// Load toolbar icons
	wxString shareLocn = GetPluginDataDir(PLUGIN_PACKAGE_NAME) + wxFileName::GetPathSeparator() + _T("data") + wxFileName::GetPathSeparator();
	
	wxString normalIcon = shareLocn + _T("autopilot_icon_normal.svg");
	wxString toggledIcon = shareLocn + _T("autopilot_icon_toggled.svg");
	wxString rolloverIcon = shareLocn + _T("autopilot_icon_rollover.svg");

	// Insert the toolbar icons
	autopilotToolbar = InsertPlugInToolSVG(_T(""), normalIcon, rolloverIcon, toggledIcon, wxITEM_CHECK, _("Fusion Media Player"), _T(""), NULL, -1, 0, this);

	// Instantiate the autopilot dialog
	autopilotDialog = new  AutopilotDialog(parentWindow, this);

	// Display the default/previously saved values
	// autopilotDialog->SetModel(autopilotModel);


	// Wire up the event handler to receive events from the dialog
	Connect(wxEVT_AUTOPILOT_DIALOG_EVENT, wxCommandEventHandler(AutopilotPlugin::OnDialogEvent));
	
	// BUG BUG REMOVE
	// Initialize UDP socket for debug spew
	addrLocal.Hostname();
	addrPeer.Hostname("127.0.0.1");
	addrPeer.Service(3001);

	debugSocket = new wxDatagramSocket(addrLocal, wxSOCKET_NONE);
	
	if (!debugSocket->IsOk()) {
		wxLogMessage(_T("ERROR: failed to create UDP peer socket"));
	}

	// Notify OpenCPN what events we want to receive callbacks for
	return (WANTS_CONFIG | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_PLUGIN_MESSAGING | USES_AUI_MANAGER | WANTS_LATE_INIT);
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
	auiManager->AddPane(autopilotDialog, paneInfo);
	auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(AutopilotPlugin::OnPaneClose), NULL, this);
	auiManager->Update();

	//paneInfo = auiManager->GetPane(_T("TwoCan Autopilot Controller"));
	//if (paneInfo.IsOk()) {
		//auiManager->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(AutopilotPlugin::OnPaneClose), NULL, this);
	//}
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
		configSettings->Write(_T("Model"), autopilotModel);
	}

	// BUG BUG DEBUG REMOVE
	debugSocket->Close();
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
		return autopilotLogo;
}

// We install one toolbar items
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
	wxAuiPaneInfo paneInfo;
	paneInfo = auiManager->GetPane(_T("TwoCan Autopilot Controller"));
	autopilotDialogVisible = paneInfo.IsShown();
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

// Keep the toolbar in synch with the pane state (user has closed the dialog from the "x" button
void AutopilotPlugin::OnPaneClose(wxAuiManagerEvent& event) {
	// BUG BUG Do I need to check that the event belongs to our pane/window ?
	autopilotDialogVisible = false;
	SetToolbarItemState(autopilotToolbar, autopilotDialogVisible);
}

// Receive the OpenCPN Plugin Messages. Only interested in message id "TWOCAN_AUTOPILOT_RESPONSE"
void AutopilotPlugin::SetPluginMessage(wxString &message_id, wxString &message_body) {

	// Brief description of the JSON schema used by TwoCan
	
	// Autopilot
	// root["autopilot"]["model"]
	// root["autopilot"]["mode"]
	// root["autopilot"]["heading"]
	// BUG BUG ToDo
	
	wxJSONReader jsonReader;
	wxJSONWriter jsonWriter;
	wxJSONValue root;

	if (message_id == _T("TWOCAN_AUTOPILOT_RESPONSE")) {
		if (jsonReader.Parse(message_body, &root) > 0) {
			// Save the erroneous json text for debugging
			wxLogMessage("TwoCan Autopilot, JSON Error in following text:");
			wxLogMessage("%s", message_body);
			wxArrayString jsonErrors = jsonReader.GetErrors();
			for (auto it : jsonErrors) {
				wxLogMessage(it);
			}
			return;
		}

		// BUG BUG DEBUG REMOVE
		debugSocket->SendTo(addrPeer, message_body.data(), message_body.Length());

		// BUG BUG ToDo
	}
}

// Handle events from the dialog
// Encode the JSON commands to send to the twocan autopilot device so it can generate the NMEA 2000 messages
void AutopilotPlugin::OnDialogEvent(wxCommandEvent &event) {
	wxString message_body;
	wxJSONValue root;
	wxJSONWriter writer;
	switch (event.GetId()) {
		case AUTOPILOT_ON: 
			root["autopilot"]["on"] = (event.GetString() == "On") ? true : false;
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_STANDBY: 
			root["autopilot"]["standby"] = (event.GetString() == "Standby") ? true : false;
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_MODE_HEADING: 
			//zone0Volume = std::atoi(event.GetString());
			root["autopilot"]["mode"] = "HEADING";
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_MODE_WIND:
			//zone1Volume = std::atoi(event.GetString());
			root["autopilot"]["mode"] = "WIND";
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_MODE_NAV:
			root["autopilot"]["mode"] = "NAV";
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_TACK_PORT:
			root["autopilot"]["tack"] = "PORT";
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_TACK_STBD:
			root["autopilot"]["tack"] = "STBD";
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_HEADING_PLUS_ONE:
			root["autopilot"]["heading"] = std::atoi(event.GetString());
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_HEADING_MINUS_ONE:
			root["autopilot"]["heading"] = std::atoi(event.GetString());
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_HEADING_PLUS_TEN:
			root["autopilot"]["heading"] = std::atoi(event.GetString());
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		case AUTOPILOT_HEADING_MINUS_TEN:
			root["autopilot"]["heading"] = std::atoi(event.GetString());
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break; 
			
		case AUTOPILOT_MODEL:
			root["autopilot"]["model"] = event.GetString();
			writer.Write(root, message_body);
			SendPluginMessage(_T("TWOCAN_AUTOPILOT_CONTROL"), message_body);
			break;

		default:
			event.Skip();
	}
}