#pragma once
#include <string>
#include "nhjson.h"


class SDKUserMoreInfo
{
public:
	bool fautoadmit;
	bool fcvrt;
	bool fban;
	std::wstring cvrtto;
	std::wstring name;

public:
	SDKUserMoreInfo();
	SDKUserMoreInfo(nlohmann::json& json);


	void SetToDefault();
	void ParseFromJSON(nlohmann::json& json);
	const nlohmann::json DumpJson();
};
