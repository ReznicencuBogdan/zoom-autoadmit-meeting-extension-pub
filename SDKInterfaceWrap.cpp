#include "stdafx.h"
#include "SDKInterfaceWrap.h"


SDKInterfaceWrap::SDKInterfaceWrap()
{
	_inited = false;
	_auth_service = NULL;
	_meeting_service = NULL;
	_meeting_participants_ctrl = NULL;
	_last_error = NULL;
}
SDKInterfaceWrap::~SDKInterfaceWrap()
{
	if (_inited)
	{
		if (_auth_service)
		{
			ZOOM_SDK_NAMESPACE::DestroyAuthService(_auth_service);
			_auth_service = NULL;
		}

		if (_meeting_service)
		{
			ZOOM_SDK_NAMESPACE::DestroyMeetingService(_meeting_service);
			_meeting_service = NULL;
		}

		if (_last_error)
		{
			_last_error = NULL;
		}

		ZOOM_SDK_NAMESPACE::CleanUPSDK();

		_inited = false;
	}
}
SDKInterfaceWrap& SDKInterfaceWrap::Instance()
{
	static SDKInterfaceWrap s_inst;
	return s_inst;
}


ZOOM_SDK_NAMESPACE::SDKError SDKInterfaceWrap::Init(ZOOM_SDK_NAMESPACE::InitParam& param_)
{
	ZOOM_SDK_NAMESPACE::SDKError err = ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS;
	if (!_inited)
	{
		err = ZOOM_SDK_NAMESPACE::InitSDK(param_);
		_inited = (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS == err) ? true : false;
	}

	return err;
}
ZOOM_SDK_NAMESPACE::IAuthService* SDKInterfaceWrap::GetAuthService()
{
	if (_inited && NULL == _auth_service)
	{
		ZOOM_SDK_NAMESPACE::CreateAuthService(&_auth_service);
		if (_auth_service)
		{
			_auth_service->SetEvent(this);
		}
	}

	return _auth_service;
}
ZOOM_SDK_NAMESPACE::IMeetingService* SDKInterfaceWrap::GetMeetingService()
{
	if (_inited && NULL == _meeting_service)
	{
		ZOOM_SDK_NAMESPACE::CreateMeetingService(&_meeting_service);
		if (_meeting_service)
		{
			_meeting_service->SetEvent(this);
			GetMeetingParticipantsController();
		}
	}

	return _meeting_service;
}
ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* SDKInterfaceWrap::GetMeetingParticipantsController()
{
	if (NULL == _meeting_participants_ctrl && _inited && _meeting_service)
	{
		_meeting_participants_ctrl = _meeting_service->GetMeetingParticipantsController();

		if (_meeting_participants_ctrl)
			_meeting_participants_ctrl->SetEvent(this);
	}

	return _meeting_participants_ctrl;
}
ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomController* SDKInterfaceWrap::GetMeetingWaitingRoomController()
{
	if (NULL == _meeting_waitingroom_ctrl && _inited && _meeting_service)
	{
		_meeting_waitingroom_ctrl = _meeting_service->GetMeetingWaitingRoomController();

		if (_meeting_waitingroom_ctrl)
		{
			_meeting_waitingroom_ctrl->SetEvent(this);
		}
	}

	return _meeting_waitingroom_ctrl;
}
ZOOM_SDK_NAMESPACE::IMeetingChatController* SDKInterfaceWrap::GetMeetingChatController()
{	
	if (NULL == _meeting_char_ctrl && _inited && _meeting_service)
	{
		_meeting_char_ctrl = _meeting_service->GetMeetingChatController();

		//if (_meeting_char_ctrl)
		//{
		//	// _meeting_char_ctrl->SetEvent(this);
		//}
	}

	return _meeting_char_ctrl;
}
ZOOM_SDK_NAMESPACE::IMeetingVideoController* SDKInterfaceWrap::GetMeetingVideoController()
{
	if (NULL == _metting_video_ctrl && _inited && _meeting_service)
	{
		_metting_video_ctrl = _meeting_service->GetMeetingVideoController();
	}

	return _metting_video_ctrl;
}


const ZOOM_SDK_NAMESPACE::IZoomLastError* SDKInterfaceWrap::GetLastError()
{
	if (_inited && NULL == _last_error)
	{
		_last_error = ZOOM_SDK_NAMESPACE::GetZoomLastError();
	}
	return  _last_error;
}


void SDKInterfaceWrap::ListenInMeetingServiceMgrEvent(ISDKInMeetingServiceMgrEvent* event_)
{
	_sdk_inmeeting_service_mgr_event_pool.AddListener(event_);
}
void SDKInterfaceWrap::UnListenInMeetingServiceMgrEvent(ISDKInMeetingServiceMgrEvent* event_)
{
	_sdk_inmeeting_service_mgr_event_pool.RemoveListener(event_);
}
