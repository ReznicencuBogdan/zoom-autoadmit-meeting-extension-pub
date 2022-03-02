#pragma once
#pragma warning( disable : 26812 )
#pragma message("Compiling precompiled headers - something must've changed.\n")

#pragma warning(push, 0)   
#define wxUSE_CONTROLS 1
#define wxUSE_UNICODE 1
#define wxUSE_STL 0
#define wxUSE_STD_DEFAULT  1
#define wxUSE_STD_CONTAINERS_COMPATIBLY wxUSE_STD_DEFAULT
#define wxUSE_STD_CONTAINERS 0
#define wxUSE_STD_IOSTREAM  wxUSE_STD_DEFAULT
#define wxUSE_STD_STRING  wxUSE_STD_DEFAULT
#define wxUSE_STD_STRING_CONV_IN_WXSTRING wxUSE_STL
#define wxUSE_IMAGE 1
#define wxUSE_LIBJPEG 1
#define wxUSE_BUTTON 1

#include <wx/wxprec.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/wrapsizer.h>
#include <wx/imaglist.h>
#include <wx/imagjpeg.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/srchctrl.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/valnum.h>
#include <wx/artprov.h>
#include <wx/defs.h>
#include <wx/image.h>
#include <wx/button.h>
#include <wx/stc/stc.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#pragma warning(pop)

//std
#include <map>
#include <string>
#include <memory>
#include <cwctype>
#include <ranges>
#include <string_view>

//
#include "ZoomDataManager.h"
#include "SdkUtils.h"
#include "SDKListHelper.h"
#include "SDKInterfaceWrap.h"