#include "SDKUserDatabase.h"
#include "SDKInterfaceWrap.h"
#include "SDKListHelper.h"
#include "SDKUtils.h"
#include <fstream>
#include <fmt/format.h>

SDKUserDatabase::SDKUserDatabase() {}
SDKUserDatabase::~SDKUserDatabase() {}

SDKUserDatabase& SDKUserDatabase::GetInst()
{
	static SDKUserDatabase s_inst;
	return s_inst;
}


void SDKUserDatabase::SetPath(const wchar_t* path)
{
	g_mPath = path;
}
bool SDKUserDatabase::InitializeDatabase()
{
	std::ifstream file_stream(g_mPath);

	if (file_stream.is_open())
	{
		file_stream >> g_mJsonObject;
		g_mUserMap.clear();

		auto& json_users = g_mJsonObject["users"];

		for (auto& json_user : json_users)
		{
			SDKUserMoreInfo dobj(json_user);

			g_mUserMap.insert({dobj.name, dobj});
		}

		auto& json_config = g_mJsonObject["config"];

		g_mConfigData.userName = NarrowStrToWideStrTrunc(json_config["userName"]);
		g_mConfigData.jwt = NarrowStrToWideStrTrunc(json_config["jwt"]);
		g_mConfigData.psw = NarrowStrToWideStrTrunc(json_config["psw"]);
		g_mConfigData.meetingNumber = json_config["meetingNumber"];

		g_mJsonObject.clear();
		file_stream.close();
	}
	else
	{
		return false;
	}
	
	return true;
}


bool SDKUserDatabase::GetUserDictObject(std::wstring const& username, SDKUserMoreInfo& dobj)
{
	MapOfUserMoreInfo::iterator i = g_mUserMap.find(username);

	bool ret = false;

	if (i != g_mUserMap.end())
	{
		dobj = i->second;
		ret = true;
	}
	else
	{
		dobj.SetToDefault();
		dobj.name = username;
		g_mUserMap.insert({ username, dobj });
	}

	return ret;
}

const ConfigData& SDKUserDatabase::GetConfigData() const
{
	return g_mConfigData;
}

bool SDKUserDatabase::MergeUserListOnDisk()
{
	std::ofstream file_stream(g_mPath);

	if (file_stream.is_open())
	{
		JSONOBJECT outputJson;

		auto usersListJson = JSONOBJECT::array();

		for (const auto& usermapobject : g_mUserMap) 
		{
			SDKUserMoreInfo ss = usermapobject.second;

			usersListJson.push_back(ss.DumpJson());
		}

		outputJson["users"] = usersListJson;

		auto& json_config = g_mJsonObject["config"];

		outputJson["config"] = 
		{
			{"jwt", WideStrToNarrowStrTrunc(g_mConfigData.jwt)},
			{"meetingNumber", g_mConfigData.meetingNumber},
			{"psw", WideStrToNarrowStrTrunc(g_mConfigData.psw)},
			{"userName", WideStrToNarrowStrTrunc(g_mConfigData.userName)}
		};

		file_stream << std::setw(4) << outputJson << std::endl;

		file_stream.close();
	}
	else
	{
		return false;
	}

	return true;
}