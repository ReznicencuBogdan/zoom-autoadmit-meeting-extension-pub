#include "SDKUserVirtualMgr.h"
#include "SDKInterfaceWrap.h"
#include "SDKUserDatabase.h"

SDKUserVirtualMgr::SDKUserVirtualMgr() 
{
	m_pParticipantsCtrl = SDKInterfaceWrap::GetInst().GetMeetingParticipantsController();
}
SDKUserVirtualMgr::~SDKUserVirtualMgr() {}

SDKUserVirtualMgr& SDKUserVirtualMgr::GetInst()
{
	static SDKUserVirtualMgr s_inst;
	return s_inst;
}


ZOOM_SDK_NAMESPACE::IUserInfo* SDKUserVirtualMgr::AddUser(unsigned int userId)
{
	SDKMAP_USERID_TO_WSTRING::iterator it = m_mUserIdToUsernameMap.find(userId);

	if (it != m_mUserIdToUsernameMap.end())
	{
		//This userid is already here
		return NULL;
	}

	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo = m_pParticipantsCtrl->GetUserByUserID(userId);

	if (pUserInfo == NULL)
	{
		return NULL;
	}

	std::wstring wusername = pUserInfo->GetUserNameW();

	m_mUserIdToUsernameMap[userId] = wusername;

	SDKUserMoreInfo userInfo;
	SDKUserDatabase::GetInst().GetUserDictObject(wusername, userInfo);



	return pUserInfo;
}
ZOOM_SDK_NAMESPACE::IUserInfo* SDKUserVirtualMgr::RemoveUser(unsigned int userId)
{

}