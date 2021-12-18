#include "SDKInterfacePlugin.h"
#include "SDKUserDatabase.h"
#include "SdkUtils.h"

#define ZOOM_MAIL L"TESTZOOMCLIENT@gmail.com"
#define ZOOM_PWS L"ZOOMCLIENTPASSWORD"

SDKInterfacePlugin::SDKInterfacePlugin(ICustomUIFlow* p_eCustomUIFlow)
{
	m_pParticipantsCtrl = NULL;
	m_pWaitingRoomCtrl = NULL;
	m_pCustomUIFlow = p_eCustomUIFlow;
}
SDKInterfacePlugin::~SDKInterfacePlugin() {}


//IAuthServiceEvent
void SDKInterfacePlugin::onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret)
{
	if (ret == ZOOM_SDK_NAMESPACE::AuthResult::AUTHRET_SUCCESS)
	{
		ZOOM_SDK_NAMESPACE::LoginParam loginParam;
		loginParam.loginType = ZOOM_SDK_NAMESPACE::LoginType::LoginType_Email;
		loginParam.ut.emailLogin.userName = ZOOM_MAIL;
		loginParam.ut.emailLogin.password = ZOOM_PWS;
		loginParam.ut.emailLogin.bRememberMe = false;

		// Initiate login 
		SDKInterfaceWrap::GetInst().GetAuthService()->Login(loginParam);

		// Create IMeetingService object to perform meeting actions
		SDKInterfaceWrap::GetInst().GetMeetingService();
	}

	m_pCustomUIFlow->onAuthenticationReturn(ret);
}
void SDKInterfacePlugin::onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason)
{
	m_pCustomUIFlow->onLoginReturnWithReason(ret, pAccountInfo, reason);
}






//IMeetingParticipantsCtrlEvent
void SDKInterfacePlugin::onUserJoin(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList /*= NULL*/)
{
	SDKListHelper listHelper(lstUserID);

	auto pFilterClb = [&](ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo, unsigned int userId) -> bool
	{
		SDKUserMoreInfo userDobj;
		if (SDKUserDatabase::GetInst().GetUserDictObject(pUserInfo->GetUserNameW(), userDobj))
		{
			if (userDobj.fban)
			{
				m_pParticipantsCtrl->ExpelUser(userId);
			}
			else if (userDobj.fcvrt)
			{
				m_pParticipantsCtrl->ChangeUserName(userId, userDobj.cvrtto.c_str(), true);

				OutputDebugStringFmt(fmt::format(L"User {0} has alias {1}", pUserInfo->GetUserNameW(), userDobj.cvrtto.c_str()));
			}
			else
			{
				OutputDebugStringFmt(fmt::format(L"User {0} doesnt have an alias", pUserInfo->GetUserNameW()));
			}
		}

		return true;
	};

	listHelper.ForEachItem(pFilterClb);
}
void SDKInterfacePlugin::onUserLeft(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList /*= NULL*/)
{
}
void SDKInterfacePlugin::onUserNameChanged(unsigned int userId, const wchar_t* userName)
{
	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo = m_pParticipantsCtrl->GetUserByUserID(userId);
	
	if (pUserInfo)
	{
		OutputDebugStringFmt(fmt::format(L"changed username of {0} to {1}", pUserInfo->GetUserNameW(), userName));
	}
}

//IMeetingWaitingRoomEvent
void SDKInterfacePlugin::onWatingRoomUserJoin(unsigned int userID)
{
	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfoWaitRoom = m_pWaitingRoomCtrl->GetWaitingRoomUserInfoByID(userID);

	if (pUserInfoWaitRoom == NULL || pUserInfoWaitRoom->IsMySelf())
	{
		return;
	}

	const wchar_t* wusername = pUserInfoWaitRoom->GetUserNameW();

	OutputDebugStringFmt(fmt::format(L"User {0} is in waiting room", wusername));

	// What to do if there is another user already logged in ?
	// Then I wont allow this one to enter, rather I will send a message in chat 
	// to alert the admins.
	SDKListHelper listHelper(m_pParticipantsCtrl->GetParticipantsList());
	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo = listHelper.GetUserInfoByName(wusername);

	if (pUserInfo == NULL)
	{
		SDKUserMoreInfo userDobj;
		const wchar_t* pusername = pUserInfoWaitRoom->GetUserNameW();
		if (SDKUserDatabase::GetInst().GetUserDictObject(pusername, userDobj))
		{
			if (userDobj.fautoadmit && !userDobj.fban)
			{
				m_pWaitingRoomCtrl->AdmitToMeeting(userID);
				OutputDebugStringFmt(fmt::format(L"User {0} is allowed to enter", pusername));
			}
			else
			{
				OutputDebugStringFmt(fmt::format(L"User {0} WILL REMAIN IN WAITING ROOM", pusername));
			}
		}
	}
	else
	{
		std::wstring strChatMessage = fmt::format(L"O persoana cu numele '{0}' exista deja in lista! Ai grija la conflicte!\n Nota: A doua instanta nu a fost admisa!", pUserInfoWaitRoom->GetUserNameW());
		OutputDebugStringFmt(fmt::format(L"Sending warning message:'{0}' to following co-hosts:", strChatMessage));

		auto pFilterClb = [&](ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo, unsigned int userId) -> bool
		{
			ZOOM_SDK_NAMESPACE::UserRole thisUserRole = pUserInfo->GetUserRole();
			if (ZOOM_SDK_NAMESPACE::UserRole::USERROLE_COHOST == thisUserRole ||
				ZOOM_SDK_NAMESPACE::UserRole::USERROLE_HOST == thisUserRole)
			{
				OutputDebugStringFmt(fmt::format(L"---> co-host:{0}", pUserInfo->GetUserNameW()));

				m_pChatCtrl->SendChatMsgTo(
					const_cast<wchar_t*>(strChatMessage.c_str()),
					pUserInfo->GetUserID(),
					ZOOM_SDK_NAMESPACE::SDKChatMessageType::SDKChatMessageType_To_Individual);
			}
			return true;
		};

		listHelper.ForEachItem(pFilterClb);
	}
}
void SDKInterfacePlugin::onWatingRoomUserLeft(unsigned int userID) {}




bool SDKInterfacePlugin::InitializeZoomPlugin()
{
	const ConfigData& config = SDKUserDatabase::GetInst().GetConfigData();

	ZOOM_SDK_NAMESPACE::InitParam initParam;
	initParam.strWebDomain = L"https://zoom.us";

	ZOOM_SDK_NAMESPACE::AuthContext authContext;
	authContext.jwt_token = config.jwt.c_str();

	if (SDKInterfaceWrap::GetInst().Init(initParam) != ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS)
	{
		return false;
	}

	// Init auth service if not already
	ZOOM_SDK_NAMESPACE::IAuthService* authService = SDKInterfaceWrap::GetInst().GetAuthService();
	SDKInterfaceWrap::GetInst().ListenInMeetingServiceMgrEvent(this);

	if (authService == NULL)
	{
		return false;
	}

	if (authService->SDKAuth(authContext) != ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS)
	{
		return false;
	}

	return true;
}
bool SDKInterfacePlugin::JoinMeetingPluginUser()
{
	const ConfigData& config = SDKUserDatabase::GetInst().GetConfigData();

	// Join meeting for end user with JoinParam object
	ZOOM_SDK_NAMESPACE::JoinParam joinParam;
	joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_NORMALUSER;
	joinParam.param.normaluserJoin.meetingNumber = config.meetingNumber;
	joinParam.param.normaluserJoin.psw = config.psw.c_str();
	joinParam.param.normaluserJoin.userName = config.userName.c_str();
	joinParam.param.normaluserJoin.vanityID = NULL;
	joinParam.param.normaluserJoin.hDirectShareAppWnd = NULL;
	joinParam.param.normaluserJoin.customer_key = NULL;
	joinParam.param.normaluserJoin.webinarToken = NULL;
	joinParam.param.normaluserJoin.isVideoOff = false;
	joinParam.param.normaluserJoin.isAudioOff = false;
	joinParam.param.normaluserJoin.isDirectShareDesktop = false;

	ZOOM_SDK_NAMESPACE::IMeetingService* meetingService = SDKInterfaceWrap::GetInst().GetMeetingService();

	if (meetingService->Join(joinParam) != ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS)
	{
		return false;
	}

	m_pParticipantsCtrl = SDKInterfaceWrap::GetInst().GetMeetingParticipantsController();

	if (m_pParticipantsCtrl == NULL)
	{
		return false;
	}

	m_pWaitingRoomCtrl = SDKInterfaceWrap::GetInst().GetMeetingWaitingRoomController();

	if (m_pWaitingRoomCtrl == NULL)
	{
		return false;
	}

	m_pChatCtrl = SDKInterfaceWrap::GetInst().GetMeetingChatController();

	if (m_pChatCtrl == NULL)
	{
		return false;
	}

	return true;
}
