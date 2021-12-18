#include "SDKUserMoreInfo.h"
#include "SDKUtils.h"


SDKUserMoreInfo::SDKUserMoreInfo()
{
	SetToDefault();
}

SDKUserMoreInfo::SDKUserMoreInfo(nlohmann::json& json)
{
	ParseFromJSON(json);
}


void SDKUserMoreInfo::SetToDefault()
{
	fautoadmit = false;
	fcvrt = false;
	fban = false;
}

void SDKUserMoreInfo::ParseFromJSON(nlohmann::json& json)
{
	auto _cvrtto = NarrowStrToWideStrTrunc(json["cvrtto"]);
	auto _name = NarrowStrToWideStrTrunc(json["name"]);

	cvrtto =_cvrtto;
	name = _name;
	fcvrt = json["fcvrt"];
	fautoadmit = json["fautoadmit"];
	fban = json["fban"];
}

const nlohmann::json SDKUserMoreInfo::DumpJson()
{
	std::string _name = WideStrToNarrowStrTrunc(name);
	std::string _cvrtto = WideStrToNarrowStrTrunc(cvrtto);

	return
	{
		{"fautoadmit", fautoadmit},
		{"fcvrt", fcvrt},
		{"fban", fban},
		{"cvrtto", _cvrtto},
		{"name", _name},
	};
}