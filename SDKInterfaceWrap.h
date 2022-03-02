#pragma once
#include "SDKUtils.h"

#define Pool_Select_Call(T_Name, EventSinkPool_Name, FUNC) \
{ \
	EventSinkPool_Name._event_sink_lock.Lock();\
	std::set<T_Name* > tmp_set;\
	tmp_set.insert(EventSinkPool_Name._event_sink_pool.begin(), EventSinkPool_Name._event_sink_pool.end());\
	std::set<T_Name* >::iterator iter_ = tmp_set.begin();\
	for (;tmp_set.end() != iter_; iter_++)\
	{\
		if (*iter_) (*iter_)->FUNC;\
	}\
	EventSinkPool_Name._event_sink_lock.Unlock();\
}


template<class T>
class EventSinkPool
{
public:
	EventSinkPool()
	{

	}
	virtual ~EventSinkPool()
	{
		_event_sink_lock.Lock();
		_event_sink_pool.clear();
		_event_sink_lock.Unlock();
	}

	void AddListener(T* sink_)
	{
		if (sink_)
		{
			_event_sink_lock.Lock();
			_event_sink_pool.insert(sink_);
			_event_sink_lock.Unlock();
		}
	}

	void RemoveListener(T* sink_)
	{
		if (sink_)
		{
			_event_sink_lock.Lock();
			_event_sink_pool.erase(sink_);
			_event_sink_lock.Unlock();
		}
	}

public:
	CComAutoCriticalSection _event_sink_lock;
	std::set<T* > _event_sink_pool;
};


class ISDKInMeetingServiceMgrEvent
	: public ZOOM_SDK_NAMESPACE::IAuthServiceEvent
	, public ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent
	, public ZOOM_SDK_NAMESPACE::IMeetingServiceEvent
	, public ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomEvent
{
public:
	virtual ~ISDKInMeetingServiceMgrEvent() {}
};


class SDKInterfaceWrap
	: public ZOOM_SDK_NAMESPACE::IAuthServiceEvent
	, public ZOOM_SDK_NAMESPACE::IMeetingParticipantsCtrlEvent
	, public ZOOM_SDK_NAMESPACE::IMeetingServiceEvent
	, public ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomEvent
{
public:
	static SDKInterfaceWrap& Instance();

	ZOOM_SDK_NAMESPACE::SDKError Init(ZOOM_SDK_NAMESPACE::InitParam& param_);
	ZOOM_SDK_NAMESPACE::IAuthService* GetAuthService();
	ZOOM_SDK_NAMESPACE::IMeetingService* GetMeetingService();
	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* GetMeetingParticipantsController();
	ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomController* GetMeetingWaitingRoomController();
	ZOOM_SDK_NAMESPACE::IMeetingChatController* GetMeetingChatController();
	ZOOM_SDK_NAMESPACE::IMeetingVideoController* GetMeetingVideoController();
	const ZOOM_SDK_NAMESPACE::IZoomLastError* GetLastError();

	void ListenInMeetingServiceMgrEvent(ISDKInMeetingServiceMgrEvent* event_);
	void UnListenInMeetingServiceMgrEvent(ISDKInMeetingServiceMgrEvent* event_);
private:
	SDKInterfaceWrap();
	virtual ~SDKInterfaceWrap();

private:
	EventSinkPool<ISDKInMeetingServiceMgrEvent> _sdk_inmeeting_service_mgr_event_pool;

	ZOOM_SDK_NAMESPACE::IAuthService* _auth_service;
	ZOOM_SDK_NAMESPACE::IMeetingService* _meeting_service;
	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* _meeting_participants_ctrl;
	ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomController* _meeting_waitingroom_ctrl;
	ZOOM_SDK_NAMESPACE::IMeetingChatController* _meeting_char_ctrl;
	ZOOM_SDK_NAMESPACE::IMeetingVideoController* _metting_video_ctrl;

private:
	const ZOOM_SDK_NAMESPACE::IZoomLastError* _last_error;
	bool _inited;

public:
	//IAuthServiceEvent
	virtual void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onAuthenticationReturn(ret))
	}
	virtual void onLogout()
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onLogout())
	}
	virtual void onZoomIdentityExpired()
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onZoomIdentityExpired())
	}
	virtual void onZoomAuthIdentityExpired()
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onZoomAuthIdentityExpired())
	}
	virtual void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onLoginReturnWithReason(ret, pAccountInfo, reason))
	}

	//IMeetingParticipantsCtrlEvent
	virtual void onUserJoin(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList = NULL)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserJoin(lstUserID, strUserList))
	}
	virtual void onUserLeft(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList = NULL)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserLeft(lstUserID, strUserList))
	}
	virtual void onHostChangeNotification(unsigned int userId)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onHostChangeNotification(userId))
	}
	virtual void onLowOrRaiseHandStatusChanged(bool bLow, unsigned int userid)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onLowOrRaiseHandStatusChanged(bLow, userid))
	}
	virtual void onUserNameChanged(unsigned int userId, const wchar_t* userName)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onUserNameChanged(userId, userName))
	}
	virtual void onCoHostChangeNotification(unsigned int userId, bool isCoHost)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onCoHostChangeNotification(userId, isCoHost))
	}
	virtual void onInvalidReclaimHostkey() {}

	//IMeetingServiceEvent
	virtual void onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int iResult = 0)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onMeetingStatusChanged(status, iResult))
	}
	virtual void onMeetingStatisticsWarningNotification(ZOOM_SDK_NAMESPACE::StatisticsWarningType type) {}
	virtual void onMeetingParameterNotification(const ZOOM_SDK_NAMESPACE::MeetingParameter* meeting_param) {}

	//IMeetingWaitingRoomEvent
	virtual void onWatingRoomUserJoin(unsigned int userID)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onWatingRoomUserJoin(userID))
	}
	virtual void onWatingRoomUserLeft(unsigned int userID)
	{
		Pool_Select_Call(ISDKInMeetingServiceMgrEvent, _sdk_inmeeting_service_mgr_event_pool, onWatingRoomUserLeft(userID))
	}
};
