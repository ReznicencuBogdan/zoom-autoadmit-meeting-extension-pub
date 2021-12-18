#pragma once
#include "SDKInterfaceWrap.h"
#include "SDKListHelper.h"
#include <memory>

class ICustomUIFlow
{
public:
	//IAuthServiceEvent
	virtual void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret) = 0;
	virtual void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) = 0;
};


class SDKInterfacePlugin : public ISDKInMeetingServiceMgrEvent
{
public:
	SDKInterfacePlugin(ICustomUIFlow* p_eCustomUIFlow);
	virtual ~SDKInterfacePlugin();

private:
	SDKInterfacePlugin() = delete;

public:
	bool InitializeZoomPlugin();
	bool JoinMeetingPluginUser();

public:
	//IAuthServiceEvent
	virtual void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret);
	virtual void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason);
	virtual void onLogout() {}
	virtual void onZoomIdentityExpired() {}
	virtual void onZoomAuthIdentityExpired() {}

	//IMeetingServiceEvent
	virtual void onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int iResult = 0) {}
	virtual void onMeetingStatisticsWarningNotification(ZOOM_SDK_NAMESPACE::StatisticsWarningType type) {}
	virtual void onMeetingParameterNotification(const ZOOM_SDK_NAMESPACE::MeetingParameter* meeting_param) {}

	//IMeetingParticipantsCtrlEvent
	virtual void onUserJoin(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList = NULL);
	virtual void onUserLeft(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList = NULL);
	virtual void onUserNameChanged(unsigned int userId, const wchar_t* userName);
	virtual void onHostChangeNotification(unsigned int userId) {}
	virtual void onLowOrRaiseHandStatusChanged(bool bLow, unsigned int userid) {}
	virtual void onCoHostChangeNotification(unsigned int userId, bool isCoHost) {}
	virtual void onInvalidReclaimHostkey() {}

	//IMeetingWaitingRoomEvent
	virtual void onWatingRoomUserJoin(unsigned int userID);
	virtual void onWatingRoomUserLeft(unsigned int userID);

private:
	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* m_pParticipantsCtrl;
	ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomController* m_pWaitingRoomCtrl;
	ZOOM_SDK_NAMESPACE::IMeetingChatController* m_pChatCtrl;

private:
	ICustomUIFlow* m_pCustomUIFlow;
};

