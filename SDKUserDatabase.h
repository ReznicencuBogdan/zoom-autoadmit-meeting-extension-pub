#pragma once
#include <unordered_set>
#include "SDKUserMoreInfo.h"
#include "nhjson.h"

typedef nlohmann::json JSONOBJECT;
typedef std::map<std::wstring, SDKUserMoreInfo> MapOfUserMoreInfo;

struct ConfigData
{
	std::wstring userName;
	std::wstring jwt;
	std::wstring psw;
	unsigned long long meetingNumber;
};

class SDKUserDatabase
{
public:
	static SDKUserDatabase& GetInst();

public:
	void SetPath(const wchar_t* path);
	bool InitializeDatabase();
	bool GetUserDictObject(std::wstring const& username, SDKUserMoreInfo& dobj);
	const ConfigData& GetConfigData() const;
	bool MergeUserListOnDisk();
private:
	SDKUserDatabase();
	~SDKUserDatabase();

private:
	MapOfUserMoreInfo g_mUserMap;
	JSONOBJECT g_mJsonObject;
	ConfigData g_mConfigData;
	const wchar_t* g_mPath;
};

