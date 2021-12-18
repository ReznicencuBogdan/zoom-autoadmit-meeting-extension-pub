#pragma once
#include "SDKUtils.h"


class SDKListHelper
{
public:
	explicit SDKListHelper(ZOOM_SDK_NAMESPACE::IList<unsigned int>* lstUserID);

	template <class T>
	SDKListHelper(T) = delete;

	SDKListHelper(const SDKListHelper&) = delete;

	void ForEachItem(std::function<bool(ZOOM_SDK_NAMESPACE::IUserInfo*, unsigned int userId)> lambda);

	ZOOM_SDK_NAMESPACE::IUserInfo* GetUserInfoByName(const wchar_t* username);

private:
	ZOOM_SDK_NAMESPACE::IList<unsigned int>* m_pLstUserId;
};

