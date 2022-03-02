#include "stdafx.h"
#include "SDKListHelper.h"
#include "SDKInterfaceWrap.h"


SDKListHelper::SDKListHelper(ZOOM_SDK_NAMESPACE::IList<unsigned int>* lstUserID)
{
	m_pLstUserId = lstUserID;
}


void SDKListHelper::ForEachItem(const std::function<bool(ZOOM_SDK_NAMESPACE::IUserInfo*, unsigned int)>& lambda)
{
	if (NULL == m_pLstUserId)
	{
		return;
	}

	int count = m_pLstUserId->GetCount();

	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* pctrl = SDKInterfaceWrap::Instance().GetMeetingParticipantsController();

	for (int i = 0; i < count; i++)
	{
		unsigned int userId = m_pLstUserId->GetItem(i);

		ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo = nullptr;

		pUserInfo = pctrl->GetUserByUserID(userId);

		if (pUserInfo == NULL)
		{
			continue;
		}

		if (!lambda(pUserInfo, userId))
		{
			return;
		}
	}
}

ZOOM_SDK_NAMESPACE::IUserInfo* SDKListHelper::GetUserInfoByName(const wchar_t* username)
{
	ZOOM_SDK_NAMESPACE::IUserInfo* pRetUserInfo = NULL;

	auto pFilterClb = [&](ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo, unsigned int userId) -> bool
	{
		if (wcscmp(username, pUserInfo->GetUserNameW()) == 0)
		{
			pRetUserInfo = pUserInfo;

			//stop looping
			return false;
		}

		return true;
	};

	ForEachItem(pFilterClb);

	return pRetUserInfo;
}