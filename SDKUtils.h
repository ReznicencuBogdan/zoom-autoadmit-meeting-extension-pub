#pragma once
#include <Windows.h>
#include <vector>
#include <map>
#include <set>
#include <fmt/format.h>

#include <zoom_sdk.h>
#include <auth_service_interface.h>
#include <meeting_service_interface.h>

#include "zoom_sdk_ext.h"
#include "direct_share_helper_interface.h"
#include "network_connection_handler_interface.h"
#include "zoom_sdk_sms_helper_interface.h"

#include "meeting_service_components/meeting_annotation_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_chat_interface.h"
#include "meeting_service_components/meeting_configuration_interface.h"
#include "meeting_service_components/meeting_h323_helper_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "meeting_service_components/meeting_phone_helper_interface.h"
#include "meeting_service_components/meeting_recording_interface.h"
#include "meeting_service_components/meeting_remote_ctrl_interface.h"
#include "meeting_service_components/meeting_sharing_interface.h"
#include "meeting_service_components/meeting_ui_ctrl_interface.h"
#include "meeting_service_components/meeting_video_interface.h"
#include "meeting_service_components/meeting_waiting_room_interface.h"
#include "meeting_service_components/meeting_closedcaption_interface.h"
#include "customized_ui/customized_local_recording.h"
#include "setting_service_interface.h"
#include "customized_ui/customized_ui_mgr.h"
#include "customized_ui/customized_video_container.h"
#include "customized_ui/zoom_customized_ui.h"
#include "customized_ui/customized_share_render.h"
#include "customized_ui/customized_annotation.h"
#include <atlbase.h>
#include <WinUser.h>



inline void OutputDebugStringFmt(std::wstring const& wstr)
{
	OutputDebugString(wstr.c_str());
}


inline std::wstring NarrowStrToWideStrTrunc(std::string const& nstr)
{
	return std::wstring(nstr.begin(), nstr.end());
}

inline std::string WideStrToNarrowStrTrunc(std::wstring const& widestr)
{
	std::string nstr(widestr.length(), 0);
	std::transform(widestr.begin(), widestr.end(), nstr.begin(), [](wchar_t c) {
		return (char)c;
		});
	return nstr;
}