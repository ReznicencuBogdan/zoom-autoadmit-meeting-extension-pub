#include "stdafx.h"

//
//#define _STRINGIFY(x) #x
//#define STRINGIFY(x) _STRINGIFY(x)
//

#define wxUSE_DPI_AWARE_MANIFEST

//SetProcessDPIAware to force dpi awareness
typedef BOOL(WINAPI* SetProcessDPIAwareFunc)();

//
class DialogUser : public wxDialog
{
public:
	DialogUser() : wxDialog(NULL, wxID_ANY, wxT("Create new user entry"))
	{
		wxFlexGridSizer* flexSizer = new wxFlexGridSizer(2);
		wxIntegerValidator<int> groupValidator(&group, wxNUM_VAL_ZERO_AS_BLANK);

#define DEFINE_DIALOG_CONTROL(TYPE,NAME,LABEL,PROP) \
		flexSizer->Add(new wxStaticText(this, wxID_ANY, LABEL), 0, wxALL | wxALIGN_RIGHT, 5); \
		NAME = new TYPE(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(350, -1), PROP); \
		flexSizer->Add(NAME, 0, wxALL, 5)

		DEFINE_DIALOG_CONTROL(wxTextCtrl, txtUsername, wxT("&Real username:"), 0);
		DEFINE_DIALOG_CONTROL(wxTextCtrl, txtAlias, wxT("&Alias list:"), wxTE_MULTILINE | wxTE_CHARWRAP);
		DEFINE_DIALOG_CONTROL(wxTextCtrl, txtImagePath, wxT("&Image Path:"), 0);
		DEFINE_DIALOG_CONTROL(wxTextCtrl, txtGroup, wxT("&Groupe:"), 0);

		int txtAliasWidth = txtAlias->GetSize().GetWidth();
		txtAlias->SetMinSize(wxSize(txtAliasWidth, 80));

		txtGroup->SetValidator(groupValidator);

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		mainSizer->Add(flexSizer, 0, wxALL, 5);
		mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL, 5);

		SetSizerAndFit(mainSizer);
		Centre();
	}
public:
	wxTextCtrl* txtUsername;
	wxTextCtrl* txtAlias;
	wxTextCtrl* txtImagePath;
	wxTextCtrl* txtGroup;
	int group = -1;
};



//Custom class for virtual listview
class cwxUserVirtualListView : public wxListCtrl
{
public:
	struct StatusAttrForColors;

	cwxUserVirtualListView(wxWindow* parent, wxWindowID id = wxID_ANY)
		: wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxLC_SINGLE_SEL | wxNO_BORDER)
	{
		StatusAttrForColors::Initialize();

		this->InsertColumn(0, "# Image");
		this->InsertColumn(1, "Zoom Username");
		this->InsertColumn(2, "Alias List");
		this->InsertColumn(3, "Admit");
		this->InsertColumn(4, "Ban");
		this->InsertColumn(5, "Stranger");
		this->InsertColumn(6, "Group");

		this->SetColumnWidth(0, 200);
		this->SetColumnWidth(1, 130);
		this->SetColumnWidth(2, 170);
		this->SetColumnWidth(3, 70);
		this->SetColumnWidth(4, 70);
		this->SetColumnWidth(5, 70);
		this->SetColumnWidth(6, 70);


		userImageList = new wxImageList(200, 150, false);
		this->AssignImageList(userImageList, wxIMAGE_LIST_SMALL);

		this->Bind(wxEVT_LIST_COL_CLICK, [this](wxListEvent& ev) {
			this->SortByColumn(ev.GetColumn());
			this->Refresh();
			this->sortAscend = !this->sortAscend;
			});
	}

	struct StatusAttrForColors
	{
		static void Initialize() noexcept
		{
			online.SetBackgroundColour(wxColor(20, 232, 126));
			banned.SetTextColour(wxColor(255, 20, 80));
		}
		static inline wxListItemAttr online;
		static inline wxListItemAttr banned;
		static inline wxListItemAttr normal;
	};


	wxListItemAttr* OnGetItemAttr(long item) const override
	{
		const UserDataItem* data;

		if (flagDrawFilteredResults)
		{
			data = userDataListFiltered[item];
		}
		else
		{
			data = &MemoryUserDataManager::Instance()->GetAtUnsafe(item);
		}

		if (data->ban)
		{
			return const_cast<wxListItemAttr*>(&StatusAttrForColors::banned);
		}
		else if (data->online)
		{
			return const_cast<wxListItemAttr*>(&StatusAttrForColors::online);
		}

		return const_cast<wxListItemAttr*>(&StatusAttrForColors::normal);
	}

	wxString OnGetItemText(long item, long column) const override
	{
		const UserDataItem* data;

		if (flagDrawFilteredResults)
		{
			data = userDataListFiltered[item];
		}
		else
		{
			data = &MemoryUserDataManager::Instance()->GetAtUnsafe(item);
		}

		switch (column)
		{
		case 0: return wxT("");
		case 1: return data->username;
		case 2: return data->alias;
		case 3: return data->admit ? wxT("True") : wxT("False");
		case 4: return data->ban ? wxT("True") : wxT("False");
		case 5: return data->stranger ? wxT("True") : wxT("False");
		case 6: return std::to_wstring(data->ugroup);
		default: return wxT("");
		}
	}
	int OnGetItemImage(long item) const override
	{
		const auto& data = MemoryUserDataManager::Instance()->GetAtUnsafe(item);
		return data.imageIndex;
	}

	void AddUserDataMustRefreshManualyAfterwards(UserDataItem& data)
	{
		static const std::wstring dirpath = L".\\assets\\";
		wxBitmap oneBitmap(dirpath + data.imagePath, wxBITMAP_TYPE_JPEG);
		data.imageIndex = userImageList->Add(oneBitmap);
		long storeSize = 0;

		MemoryUserDataManager::Instance()->AddInStore(data, storeSize);
		this->SetItemCount(storeSize);
	}
	void DeleteUserData(UserDataItem& item, long index)
	{
		if (flagDrawFilteredResults)
		{
			const auto& end = std::remove_if(userDataListFiltered.begin(), userDataListFiltered.end(), [&item](const UserDataItem* vitem) noexcept
				{
					return vitem->primary == item.primary;
				});
			userDataListFiltered.erase(end, userDataListFiltered.end());
			this->SetItemCount(userDataListFiltered.size());
		}
		else
		{
			long finalSize = 0;
			MemoryUserDataManager::Instance()->EraseIf([&item](const UserDataItem& vitem) noexcept
				{
					return vitem.primary == item.primary;
				}, finalSize);

			this->SetItemCount(finalSize);
		}

		this->RefreshItem(index);
	}
	void FilterItemsByString(const std::wstring& filter)
	{
		//Clear all filter data
		userDataListFiltered.clear();

		auto storeInstance = MemoryUserDataManager::Instance();

		if (filter.size() == 0)
		{
			flagDrawFilteredResults = false;
			this->SetItemCount(storeInstance->GetStoreSize());
			this->Refresh();
			return;
		}

		//Find items by predicate
		storeInstance->ForEach([&](UserDataItem& item)
			{
				if (IsSubstringInString(item.username, filter) || IsSubstringInString(item.alias, filter))
				{
					userDataListFiltered.push_back(&item);
				}
			});

		flagDrawFilteredResults = true;
		this->SetItemCount(userDataListFiltered.size());
		this->Refresh();
	}
	UserDataItem& GetItemAtIndex(long index)
	{
		if (flagDrawFilteredResults)
		{
			return *userDataListFiltered[index];
		}
		else
		{
			return MemoryUserDataManager::Instance()->GetAtUnsafe(index);
		}
	}
	long GetSelectedIndex()
	{
		return this->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);;
	}

private:
	void SortByColumn(int column)
	{
		static auto genericCompare = [](auto i1, auto i2, bool ascending) {
			return ascending ? i1 < i2 : i1 > i2;
		};

		if (flagDrawFilteredResults == false)
		{
			MemoryUserDataManager::Instance()->Sort([column, &ascending = this->sortAscend](const UserDataItem& i1, const UserDataItem& i2) {
				switch (column)
				{
				case 1: return genericCompare(i1.username, i2.username, ascending);
				case 2: return genericCompare(i1.alias, i2.alias, ascending);
				case 3: return genericCompare(i1.admit, i2.admit, ascending);
				case 4: return genericCompare(i1.ban, i2.ban, ascending);
				case 5: return genericCompare(i1.stranger, i2.stranger, ascending);
				case 6: return genericCompare(i1.ugroup, i2.ugroup, ascending);
				default: return false;
				}
			});
		}
	}

private:
	bool flagDrawFilteredResults = false;
	std::vector<UserDataItem*> userDataListFiltered;
	wxImageList* userImageList;
	bool sortAscend = true;
};


enum LocalId
{
	//RIBBON
	ID_RIBBON_CreateNewUser = wxID_NEW + 1,
	ID_RIBBON_EditSelectedUser,
	ID_RIBBON_DeleteSelectedUser,
	ID_RIBBON_ToggleAutoAdmit,
	ID_RIBBON_ToggleBan,
	ID_RIBBON_ToggleStranger,

	//
	NUMBER_UPDATE_ID,

};

class MyApp : public wxApp
{
public: virtual bool OnInit();
};
wxIMPLEMENT_APP(MyApp);


class MainWindow final :
	public wxFrame,
	public ISDKInMeetingServiceMgrEvent
{
private:
	void onBtnSignIn_Click(wxCommandEvent&);
	void onBtnJoinMeeting_Click(wxCommandEvent&);
	void onFilterTextEnter_Press(wxCommandEvent&);
	void onListViewUsersItemSelected(wxListEvent&);
	void onListViewUsersItemClicked(wxMouseEvent&);
	void onAssignEveryoneToGroup(wxCommandEvent&);
	void onAllowAutoAdmit(wxCommandEvent&);
	void onExpelNamesWithSeparatorInside(wxCommandEvent&);

	//Ribbon button handlers
	void onCreateNewUser(wxRibbonButtonBarEvent&);
	void onEditSelectedUser(wxRibbonButtonBarEvent&);
	void onDeleteSelectedUser(wxRibbonButtonBarEvent&);
	void onToggleAutoAdmit(wxRibbonButtonBarEvent&);
	void onToggleBan(wxRibbonButtonBarEvent&);
	void onToggleStranger(wxRibbonButtonBarEvent&);
	//Spotlight
	void onSpotlightBtn_1_Click(wxCommandEvent&);
	void onSpotlightBtn_2_Click(wxCommandEvent&);
	void onSpotlightBtn_3_Click(wxCommandEvent&);
	void SpotlightUsername(const std::wstring&);

	//Interfaces
public:
	//IAuthServiceEvent
	void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret) override;
	void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) override;
	void onLogout() override {}
	void onZoomIdentityExpired() override {}
	void onZoomAuthIdentityExpired() override {}

	//IMeetingServiceEvent
	void onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int iResult = 0) override;
	void onMeetingStatisticsWarningNotification(ZOOM_SDK_NAMESPACE::StatisticsWarningType type) override {}
	void onMeetingParameterNotification(const ZOOM_SDK_NAMESPACE::MeetingParameter* meeting_param) override {}

	//IMeetingParticipantsCtrlEvent
	void onUserJoinCommon(UserDataItem& data, bool newEntry);//
	void onUserJoin(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList = NULL) override;
	void onUserLeft(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList = NULL) override;
	void onUserNameChanged(unsigned int userId, const wchar_t* userName) override;
	void onHostChangeNotification(unsigned int userId)  override {}
	void onLowOrRaiseHandStatusChanged(bool bLow, unsigned int userid) override {}
	void onCoHostChangeNotification(unsigned int userId, bool isCoHost)  override {}
	void onInvalidReclaimHostkey()  override {}

	//IMeetingWaitingRoomEvent
	void onWatingRoomUserJoin(unsigned int userID) override;
	void onWatingRoomUserLeft(unsigned int userID) override;

private:
	ZOOM_SDK_NAMESPACE::IMeetingService* meetingService;
	ZOOM_SDK_NAMESPACE::IMeetingParticipantsController* participantsController;
	ZOOM_SDK_NAMESPACE::IMeetingWaitingRoomController* waitingRoomController;
	ZOOM_SDK_NAMESPACE::IMeetingChatController* chatController;
	ZOOM_SDK_NAMESPACE::IMeetingVideoController* videoCtrl;

	ZoomConfiguration volatileData;

private:
	void AddMockUsersInListView() noexcept;
	void SetRibbonUserRelatedToolsEnabledStatus(bool flag) noexcept;
	void HandleListViewItemClickOrSelection();
	void ConfigureLogText();
	void AddLogEntry(const wxString& message, bool ok = true);
	inline bool DoesUsernameContainSeparator(const wchar_t* username);

	using RoomMap = std::map<int, const wchar_t*>;
	RoomMap GetRoomMap();

protected:
	//Ribbon
	wxRibbonBar* ribbonBar;
	//Tabs
	wxRibbonPage* homeRibbonPage;
	//wxRibbonPage* editRibbonPage;
	//MainTab panels
	wxRibbonPanel* homeRibbonPanel;
	//wxRibbonPanel* itemRibbonPanel;
	//Button holders in main panel
	wxRibbonButtonBar* homeRibbonButtonBar;
	//wxRibbonButtonBar* itemRibbonButtonBar;
	wxTextCtrl* zoomSdk;
	wxTextCtrl* zoomEmail;
	wxTextCtrl* zoomPassword;
	wxTextCtrl* zoomUsername;
	wxTextCtrl* zoomRoomId;
	wxTextCtrl* zoomRoomPassword;
	wxSearchCtrl* listUserSearch;
	cwxUserVirtualListView* listViewUsers;
	wxCheckBox* allowAutoAdmit;
	wxCheckBox* expelNamesWithSeparatorInside;
	wxTextCtrl* grSpotlight_1_p1;
	wxTextCtrl* grSpotlight_1_p2;
	wxTextCtrl* grSpotlight_2_p1;
	wxTextCtrl* grSpotlight_2_p2;
	wxTextCtrl* grSpotlight_3_p1;
	wxTextCtrl* grSpotlight_3_p2;
	wxTextCtrl* grSpotlight_Last;
	wxButton* grSpotlightBtn_1;
	wxButton* grSpotlightBtn_2;
	wxButton* grSpotlightBtn_3;
	wxStyledTextCtrl* stLogText;
	wxStatusBar* statusBar;
protected:
	ZoomConfiguration& SyncZoomConfigurationWithUIStatus();

	wxDECLARE_EVENT_TABLE();
public:
	MainWindow(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(749, 300), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);
	~MainWindow() {};

	void onNumberUpdate(wxCommandEvent& evt)
	{
		bool ok = static_cast<bool>(evt.GetInt());
		wxString message = evt.GetString();

		stLogText->SetReadOnly(false);
		stLogText->ScrollToEnd();
		stLogText->AppendText(ok ? wxT("[+] ") : wxT("[-] "));
		stLogText->AppendText(message);
		stLogText->AppendText(wxT("\n"));
		stLogText->SetReadOnly(true);
	}
};



wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
//RIBBON
EVT_RIBBONBUTTONBAR_CLICKED(LocalId::ID_RIBBON_CreateNewUser, MainWindow::onCreateNewUser)
EVT_RIBBONBUTTONBAR_CLICKED(LocalId::ID_RIBBON_EditSelectedUser, MainWindow::onEditSelectedUser)
EVT_RIBBONBUTTONBAR_CLICKED(LocalId::ID_RIBBON_DeleteSelectedUser, MainWindow::onDeleteSelectedUser)
EVT_RIBBONBUTTONBAR_CLICKED(LocalId::ID_RIBBON_ToggleAutoAdmit, MainWindow::onToggleAutoAdmit)
EVT_RIBBONBUTTONBAR_CLICKED(LocalId::ID_RIBBON_ToggleBan, MainWindow::onToggleBan)
EVT_RIBBONBUTTONBAR_CLICKED(LocalId::ID_RIBBON_ToggleStranger, MainWindow::onToggleStranger)
//THREAD
EVT_COMMAND(NUMBER_UPDATE_ID, wxEVT_COMMAND_TEXT_UPDATED, MainWindow::onNumberUpdate)
wxEND_EVENT_TABLE()



MainWindow::MainWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
	: wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxSize(450, 400));
	statusBar = this->CreateStatusBar();

	wxImage::AddHandler(new wxJPEGHandler());

	//
	wxPanel* panelLeft = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSIMPLE_BORDER);
	wxPanel* panelRight = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSIMPLE_BORDER);
	wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerBody = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizerLeft = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerRight = new wxBoxSizer(wxVERTICAL);

	//Setup the ribbon toolbar
	ribbonBar = new wxRibbonBar(this, -1, wxDefaultPosition, wxDefaultSize, wxRIBBON_BAR_FLOW_HORIZONTAL
		| wxRIBBON_BAR_SHOW_PAGE_LABELS
		| wxRIBBON_BAR_SHOW_PANEL_EXT_BUTTONS
		| wxRIBBON_BAR_SHOW_TOGGLE_BUTTON
	);
	//Tabs
	homeRibbonPage = new wxRibbonPage(ribbonBar, wxID_ANY, wxT("Zoom Main Page"), wxNullBitmap);
	//editRibbonPage = new wxRibbonPage(ribbonBar, wxID_ANY, wxT("Tools"), wxNullBitmap);
	//MainTab panels
	homeRibbonPanel = new wxRibbonPanel(homeRibbonPage, wxID_ANY, wxT("User tools"), wxNullBitmap,
		wxDefaultPosition, wxDefaultSize,
		wxRIBBON_PANEL_NO_AUTO_MINIMISE);
	/*itemRibbonPanel = new wxRibbonPanel(homeRibbonPage, wxID_ANY, wxT("Meeting tools"),
		wxNullBitmap, wxDefaultPosition, wxDefaultSize,
		wxRIBBON_PANEL_NO_AUTO_MINIMISE);*/
		//Button holders in main panel
	homeRibbonButtonBar = new wxRibbonButtonBar(homeRibbonPanel);
	//itemRibbonButtonBar = new wxRibbonButtonBar(itemRibbonPanel);


	//Buttons defined here
#define DEFINE_RIBBON_BUTTON(ID, LABEL, ART) \
	homeRibbonButtonBar->AddButton(ID, LABEL, wxArtProvider::GetBitmap(ART, wxART_TOOLBAR, wxSize(16,16)))

	DEFINE_RIBBON_BUTTON(LocalId::ID_RIBBON_CreateNewUser, wxT("Create New User"), wxART_ADD_BOOKMARK);
	DEFINE_RIBBON_BUTTON(LocalId::ID_RIBBON_EditSelectedUser, wxT("Edit Selected User"), wxART_FIND_AND_REPLACE);
	DEFINE_RIBBON_BUTTON(LocalId::ID_RIBBON_DeleteSelectedUser, wxT("Delete Selected User"), wxART_DEL_BOOKMARK);
	DEFINE_RIBBON_BUTTON(LocalId::ID_RIBBON_ToggleAutoAdmit, wxT("Toggle AutoAdmit"), wxART_HELP_SETTINGS);
	DEFINE_RIBBON_BUTTON(LocalId::ID_RIBBON_ToggleBan, wxT("Toggle Ban"), wxART_HELP_SETTINGS);
	DEFINE_RIBBON_BUTTON(LocalId::ID_RIBBON_ToggleStranger, wxT("Toggle Stranger"), wxART_HELP_SETTINGS);
	//itemRibbonButtonBar->AddButton(wxID_ANY, wxT("Daftar Guru"),
	//	wxArtProvider::GetBitmap(wxART_QUESTION, wxART_TOOLBAR, ribbonButtonSize));
	//itemRibbonButtonBar->AddButton(wxID_ANY, wxT("Tambah Guru"),
	//	wxArtProvider::GetBitmap(wxART_QUESTION, wxART_TOOLBAR, ribbonButtonSize));
	//itemRibbonButtonBar->AddButton(wxID_ANY, wxT("Daftar Siswa"),
	//	wxArtProvider::GetBitmap(wxART_QUESTION, wxART_TOOLBAR, ribbonButtonSize));
	//itemRibbonButtonBar->AddButton(wxID_ANY, wxT("Tambah Siswa"),
	//	wxArtProvider::GetBitmap(wxART_QUESTION, wxART_TOOLBAR, ribbonButtonSize));
	SetRibbonUserRelatedToolsEnabledStatus(false);

	ribbonBar->AddPageHighlight(ribbonBar->GetPageCount() - 1);
	ribbonBar->Realize();
	ribbonBar->DismissExpandedPanel();
	ribbonBar->SetArtProvider(new wxRibbonMSWArtProvider);
	sizerMain->Add(ribbonBar, 0, wxEXPAND, 5);


	//LEFT
	listViewUsers = new cwxUserVirtualListView(panelLeft, wxID_ANY);
	listViewUsers->Bind(wxEVT_LEFT_DOWN, &MainWindow::onListViewUsersItemClicked, this);
	listViewUsers->Bind(wxEVT_LIST_ITEM_SELECTED, &MainWindow::onListViewUsersItemSelected, this);
	listViewUsers->Bind(wxEVT_LIST_ITEM_DESELECTED, &MainWindow::onListViewUsersItemSelected, this);
	AddMockUsersInListView();
	listUserSearch = new wxSearchCtrl(panelLeft, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
	listUserSearch->Bind(wxEVT_TEXT_ENTER, &MainWindow::onFilterTextEnter_Press, this); //enter
	listUserSearch->Bind(wxEVT_SEARCH, &MainWindow::onFilterTextEnter_Press, this); //button press
	listUserSearch->Bind(wxEVT_TEXT, &MainWindow::onFilterTextEnter_Press, this); //text changed
	sizerLeft->Add(listUserSearch, 0, wxEXPAND | wxALL, 5);
	sizerLeft->Add(listViewUsers, 1, wxEXPAND, 0);


	//RIGHT
	ZoomConfiguration zoomData;
	zoomData.Populate();
	sizerRight->Add(new wxStaticText(panelRight, wxID_ANY, wxT("Insert zoom login and meeting details")), 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5);
#define DEFINE_ZOOM_LABEL(SIZER, TEXT) \
		SIZER->Add(new wxStaticText(panelRight, wxID_ANY, TEXT), 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5)
#define DEFINE_ZOOM_TEXTCTRL(SIZER, NAME) \
		NAME = new wxTextCtrl(panelRight, wxID_ANY, zoomData.NAME, wxDefaultPosition, wxSize(200,-1)); \
		SIZER->Add(NAME, 1, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5)

	wxFlexGridSizer* mainSizerZoomDetails = new wxFlexGridSizer(2);
	wxFlexGridSizer* leftSizerZoomDetails = new wxFlexGridSizer(2);
	wxFlexGridSizer* rightSizerZoomDetails = new wxFlexGridSizer(2);

	DEFINE_ZOOM_LABEL(leftSizerZoomDetails, "Zoom E-mail");
	DEFINE_ZOOM_TEXTCTRL(leftSizerZoomDetails, zoomEmail);
	DEFINE_ZOOM_LABEL(leftSizerZoomDetails, "Zoom Password");
	DEFINE_ZOOM_TEXTCTRL(leftSizerZoomDetails, zoomPassword);
	wxButton* btnSignIn = new wxButton(panelRight, wxID_ANY, wxT("Sign In"));
	btnSignIn->Bind(wxEVT_BUTTON, &MainWindow::onBtnSignIn_Click, this);
	leftSizerZoomDetails->Add(btnSignIn, 0, wxEXPAND | wxALL, 5);

	DEFINE_ZOOM_LABEL(rightSizerZoomDetails, "Zoom JWT SDK Token");
	DEFINE_ZOOM_TEXTCTRL(rightSizerZoomDetails, zoomSdk);
	DEFINE_ZOOM_LABEL(rightSizerZoomDetails, "Zoom Username");
	DEFINE_ZOOM_TEXTCTRL(rightSizerZoomDetails, zoomUsername);
	DEFINE_ZOOM_LABEL(rightSizerZoomDetails, "Zoom Room ID");
	DEFINE_ZOOM_TEXTCTRL(rightSizerZoomDetails, zoomRoomId);
	DEFINE_ZOOM_LABEL(rightSizerZoomDetails, "Zoom Room Password");
	DEFINE_ZOOM_TEXTCTRL(rightSizerZoomDetails, zoomRoomPassword);
	wxButton* btnJoinMeeting = new wxButton(panelRight, wxID_ANY, wxT("Join Meeting"));
	btnJoinMeeting->Bind(wxEVT_BUTTON, &MainWindow::onBtnJoinMeeting_Click, this);
	rightSizerZoomDetails->Add(btnJoinMeeting, 0, wxEXPAND | wxALL, 5);

	mainSizerZoomDetails->Add(leftSizerZoomDetails, 0, wxEXPAND | wxALL, 0);
	mainSizerZoomDetails->Add(rightSizerZoomDetails, 0, wxEXPAND | wxALL, 0);
	sizerRight->Add(mainSizerZoomDetails, 0, wxALL, 5);

	//
	allowAutoAdmit = new wxCheckBox(panelRight, wxID_ANY, wxT("Allow program to autoadmit users based on autoadmit flag."));
	sizerRight->Add(allowAutoAdmit, 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5);
	allowAutoAdmit->Bind(wxEVT_CHECKBOX, &MainWindow::onAllowAutoAdmit, this);
	expelNamesWithSeparatorInside = new wxCheckBox(panelRight, wxID_ANY, wxT("Remove users with reserved keyword."));
	sizerRight->Add(expelNamesWithSeparatorInside, 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5);
	expelNamesWithSeparatorInside->Bind(wxEVT_CHECKBOX, &MainWindow::onExpelNamesWithSeparatorInside, this);
	//
	sizerRight->Add(new wxStaticText(panelRight, wxID_ANY, wxT("Create instant spotlight groups")), 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5);
	wxFlexGridSizer* flexSizer = new wxFlexGridSizer(4);
#define DEFINE_SPOTLIGHT_CONTROL(NAME_T1, NAME_T2, NAME_B, LABEL, FUNC) \
		flexSizer->Add(new wxStaticText(panelRight, wxID_ANY, LABEL), 0, wxALL | wxALIGN_RIGHT, 5); \
		NAME_T1 = new wxTextCtrl(panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1)); \
		NAME_T2 = new wxTextCtrl(panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1)); \
		NAME_B = new wxButton(panelRight, wxID_ANY, wxT("Spotlight now"), wxDefaultPosition, wxSize(-1, -1)); \
		NAME_T1->Bind(wxEVT_LEFT_DOWN, [&](wxMouseEvent& ev) { grSpotlight_Last = NAME_T1; ev.Skip();}); \
		NAME_T2->Bind(wxEVT_LEFT_DOWN,[&](wxMouseEvent& ev) { grSpotlight_Last = NAME_T2; ev.Skip();}); \
		NAME_B->Bind(wxEVT_BUTTON, &MainWindow::FUNC, this); \
		flexSizer->Add(NAME_T1, 0, wxALL, 5); flexSizer->Add(NAME_T2, 0, wxALL, 5); flexSizer->Add(NAME_B, 0, wxALL, 5)

	DEFINE_SPOTLIGHT_CONTROL(grSpotlight_1_p1, grSpotlight_1_p2, grSpotlightBtn_1, wxT("&Spotlight group:"), onSpotlightBtn_1_Click);
	DEFINE_SPOTLIGHT_CONTROL(grSpotlight_2_p1, grSpotlight_2_p2, grSpotlightBtn_2, wxT("&Spotlight group:"), onSpotlightBtn_2_Click);
	DEFINE_SPOTLIGHT_CONTROL(grSpotlight_3_p1, grSpotlight_3_p2, grSpotlightBtn_3, wxT("&Spotlight group:"), onSpotlightBtn_3_Click);
	grSpotlight_Last = grSpotlight_1_p1;
	sizerRight->Add(flexSizer, 0, wxEXPAND | wxALL, 10);

	sizerRight->Add(new wxStaticText(panelRight, wxID_ANY, wxT("Automatically assign all users to their groups")), 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 5);
	wxButton* btnAssignEveryoneToItsGroup = new wxButton(panelRight, wxID_ANY, wxT("Assign everyone to it's group"));
	btnAssignEveryoneToItsGroup->Bind(wxEVT_BUTTON, &MainWindow::onAssignEveryoneToGroup, this);
	sizerRight->Add(btnAssignEveryoneToItsGroup, 0, wxALL, 5);

	stLogText = new wxStyledTextCtrl(panelRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
	ConfigureLogText();
	sizerRight->Add(stLogText, 1, wxEXPAND | wxALL, 5);
	AddLogEntry(wxT("Logging initialized ......."));


	//
	panelLeft->SetSizer(sizerLeft);
	panelLeft->Layout();
	sizerLeft->Fit(panelLeft);

	panelRight->SetSizer(sizerRight);
	panelRight->Layout();
	sizerRight->Fit(panelRight);

	//
	sizerBody->Add(panelLeft, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5);
	sizerBody->Add(panelRight, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 5);
	sizerMain->Add(sizerBody, 1, wxEXPAND, 0);

	this->SetSizer(sizerMain);
	this->Layout();
	this->Centre(wxBOTH);
}
ZoomConfiguration& MainWindow::SyncZoomConfigurationWithUIStatus()
{
	volatileData.zoomSdk = zoomSdk->GetValue().ToStdWstring();
	volatileData.zoomEmail = zoomEmail->GetValue().ToStdWstring();
	volatileData.zoomPassword = zoomPassword->GetValue().ToStdWstring();
	volatileData.zoomUsername = zoomUsername->GetValue().ToStdWstring();
	volatileData.zoomRoomId = zoomRoomId->GetValue().ToStdWstring();
	volatileData.zoomRoomPassword = zoomRoomPassword->GetValue().ToStdWstring();
	volatileData.allowAutoAdmit = allowAutoAdmit->IsChecked();
	volatileData.expelNamesWithSeparatorInside = expelNamesWithSeparatorInside->IsChecked();

	volatileData.Save();

	return volatileData;
}

void MainWindow::AddMockUsersInListView() noexcept
{
	std::vector<UserDataItem> collection = UserDataItem::Read();

	if (collection.size() == 0)
	{
		statusBar->SetStatusText(wxT("No user data in the database .. _debug_ will populate with mock data!"));
	}
	else
	{
		statusBar->SetStatusText(wxString::Format(wxT("Read '%Iu' user data entries!"), collection.size()));

		for (auto& item : collection)
		{
			listViewUsers->AddUserDataMustRefreshManualyAfterwards(item);
		}
	}

	listViewUsers->Refresh();
}
void MainWindow::SetRibbonUserRelatedToolsEnabledStatus(bool flag) noexcept
{
	homeRibbonButtonBar->EnableButton(LocalId::ID_RIBBON_EditSelectedUser, flag);
	homeRibbonButtonBar->EnableButton(LocalId::ID_RIBBON_DeleteSelectedUser, flag);
	homeRibbonButtonBar->EnableButton(LocalId::ID_RIBBON_ToggleAutoAdmit, flag);
	homeRibbonButtonBar->EnableButton(LocalId::ID_RIBBON_ToggleBan, flag);
	homeRibbonButtonBar->EnableButton(LocalId::ID_RIBBON_ToggleStranger, flag);
}
void MainWindow::HandleListViewItemClickOrSelection()
{
	if (listViewUsers->GetSelectedItemCount() > 0)
	{
		long index = listViewUsers->GetSelectedIndex();
		UserDataItem& item = listViewUsers->GetItemAtIndex(index);

		grSpotlight_Last->SetValue(item.username);
		SetRibbonUserRelatedToolsEnabledStatus(true);
	}
	else
	{
		SetRibbonUserRelatedToolsEnabledStatus(false);
	}
}
void MainWindow::ConfigureLogText()
{
	stLogText->SetUseTabs(true);
	stLogText->SetTabWidth(4);
	stLogText->SetIndent(4);
	stLogText->SetTabIndents(true);
	stLogText->SetBackSpaceUnIndents(true);
	stLogText->SetViewEOL(false);
	stLogText->SetViewWhiteSpace(true);
	stLogText->SetIndentationGuides(true);
	stLogText->SetReadOnly(true);
	stLogText->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
	stLogText->SetMarginMask(1, wxSTC_MASK_FOLDERS);
	stLogText->SetMarginSensitive(1, true);
	stLogText->SetProperty(wxT("fold"), wxT("1"));
	stLogText->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
	stLogText->SetMarginType(0, wxSTC_MARGIN_NUMBER);
	stLogText->SetMarginWidth(0, stLogText->TextWidth(wxSTC_STYLE_LINENUMBER, wxT("_999")));
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
	stLogText->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("BLACK")));
	stLogText->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("WHITE")));
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
	stLogText->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("BLACK")));
	stLogText->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("WHITE")));
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY);
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS);
	stLogText->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("BLACK")));
	stLogText->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("WHITE")));
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS);
	stLogText->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("BLACK")));
	stLogText->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("WHITE")));
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY);
	stLogText->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY);
	stLogText->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	stLogText->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
	stLogText->SetWrapMode(1);
	stLogText->SetUseHorizontalScrollBar(false);
}
void MainWindow::AddLogEntry(const wxString& message, bool ok)
{
	// notify the main thread
	wxCommandEvent* evt = new wxCommandEvent(wxEVT_COMMAND_TEXT_UPDATED, NUMBER_UPDATE_ID);
	evt->SetInt(static_cast<int>(ok));
	evt->SetString(message);
	this->QueueEvent(evt);
}
bool MainWindow::DoesUsernameContainSeparator(const wchar_t* username)
{
	return wcschr(username, ';') != nullptr;
}

//Ribbon button handlers
void MainWindow::onCreateNewUser(wxRibbonButtonBarEvent&)
{
	UserDataItem data;
	DialogUser dialog;

	if (dialog.ShowModal() == wxID_OK)
	{
		auto dialogImagePath = dialog.txtImagePath->GetValue().ToStdWstring();
		data.username = dialog.txtUsername->GetValue().ToStdWstring();
		data.alias = dialog.txtAlias->GetValue().ToStdWstring();
		data.ugroup = dialog.group;

		if (dialogImagePath.size() > 0)
		{
			data.imagePath = std::move(dialogImagePath);
		}

		if (data.Save())
		{
			listViewUsers->AddUserDataMustRefreshManualyAfterwards(data);
			listViewUsers->Refresh();
		}
	}
}
void MainWindow::onEditSelectedUser(wxRibbonButtonBarEvent&)
{
	long selectedIndex = listViewUsers->GetSelectedIndex();
	if (selectedIndex == -1)
	{
		return;
	}

	UserDataItem& item = listViewUsers->GetItemAtIndex(selectedIndex);
	DialogUser dialog;
	dialog.txtUsername->SetValue(item.username);
	dialog.txtAlias->SetValue(item.alias);
	dialog.txtImagePath->SetValue(item.imagePath);
	dialog.txtGroup->SetValue(std::to_wstring(item.ugroup));
	dialog.group = item.ugroup;

	if (dialog.ShowModal() == wxID_OK)
	{
		item.username = dialog.txtUsername->GetValue().ToStdWstring();
		item.alias = dialog.txtAlias->GetValue().ToStdWstring();
		item.imagePath = dialog.txtImagePath->GetValue().ToStdWstring();
		item.ugroup = dialog.group;

		item.Save();
		listViewUsers->RefreshItem(selectedIndex);
	}
}
void MainWindow::onDeleteSelectedUser(wxRibbonButtonBarEvent&)
{
	long selectedIndex = listViewUsers->GetSelectedIndex();
	if (selectedIndex == -1)
	{
		return;
	}

	UserDataItem& item = listViewUsers->GetItemAtIndex(selectedIndex);
	item.Delete();

	listViewUsers->DeleteUserData(item, selectedIndex);
}
void MainWindow::onToggleAutoAdmit(wxRibbonButtonBarEvent&)
{
	long selectedIndex = listViewUsers->GetSelectedIndex();
	if (selectedIndex == -1)
	{
		return;
	}

	UserDataItem& item = listViewUsers->GetItemAtIndex(selectedIndex);
	item.admit = !item.admit;
	item.Save();
	listViewUsers->RefreshItem(selectedIndex);
}
void MainWindow::onToggleBan(wxRibbonButtonBarEvent&)
{
	long selectedIndex = listViewUsers->GetSelectedIndex();
	if (selectedIndex == -1)
	{
		return;
	}

	UserDataItem& item = listViewUsers->GetItemAtIndex(selectedIndex);
	item.ban = !item.ban;
	item.Save();
	listViewUsers->RefreshItem(selectedIndex);
}
void MainWindow::onToggleStranger(wxRibbonButtonBarEvent&)
{
	long selectedIndex = listViewUsers->GetSelectedIndex();
	if (selectedIndex == -1)
	{
		return;
	}

	UserDataItem& item = listViewUsers->GetItemAtIndex(selectedIndex);
	item.stranger = !item.stranger;
	item.Save();
	listViewUsers->RefreshItem(selectedIndex);
}

//
void MainWindow::onFilterTextEnter_Press(wxCommandEvent& ev)
{
	auto str = ev.GetString();
	listViewUsers->FilterItemsByString(str.ToStdWstring());
	ev.Skip();
}
void MainWindow::onListViewUsersItemSelected(wxListEvent& ev)
{
	this->HandleListViewItemClickOrSelection();
	ev.Skip();
}
void MainWindow::onListViewUsersItemClicked(wxMouseEvent& ev)
{
	this->HandleListViewItemClickOrSelection();
	ev.Skip();
}
void MainWindow::onAllowAutoAdmit(wxCommandEvent&)
{
	SyncZoomConfigurationWithUIStatus();
}
void MainWindow::onExpelNamesWithSeparatorInside(wxCommandEvent&)
{
	SyncZoomConfigurationWithUIStatus();
}


MainWindow::RoomMap MainWindow::GetRoomMap()
{
	RoomMap result;
	MemoryUserDataManager::Instance()->ForEach([&](UserDataItem& data)
		{
			if (data.ugroup <= 0)
			{
				return;
			}

			result[data.ugroup] = nullptr;
		});

	return result;
}

void MainWindow::onAssignEveryoneToGroup(wxCommandEvent&)
{
	ZOOMSDK::IMeetingBOController* breakoutRoomController = meetingService->GetMeetingBOController();;
	ZOOMSDK::IBOAssistant* DATA;

	if (!breakoutRoomController)
	{
		AddLogEntry(wxT("BKROOM: Failed getting BreakoutRoom Controller"), false);
		return;
	}

	ZOOMSDK::BO_STATUS resultBOStatus = breakoutRoomController->GetBOStatus();

	switch (resultBOStatus)
	{
	case ZOOMSDK::BO_STATUS::BO_STATUS_INVALID:
		AddLogEntry(wxT("BKROOM: BO_STATUS: BO_STATUS_INVALID"));
		break;
	case ZOOMSDK::BO_STATUS::BO_STATUS_EDIT:
		AddLogEntry(wxT("BKROOM: BO_STATUS: BO_STATUS_EDIT")); // <- when simple user
		break;
	case ZOOMSDK::BO_STATUS::BO_STATUS_STARTED:
		AddLogEntry(wxT("BKROOM: BO_STATUS: BO_STATUS_STARTED"));
		break;
	case ZOOMSDK::BO_STATUS::BO_STATUS_STOPPING:
		AddLogEntry(wxT("BKROOM: BO_STATUS: BO_STATUS_STOPPING"));
		break;
	case ZOOMSDK::BO_STATUS::BO_STATUS_ENDED:
		AddLogEntry(wxT("BKROOM: BO_STATUS: BO_STATUS_ENDED"));
		break;
	default:
		break;
	}

	auto BOAdmin = breakoutRoomController->GetBOAdminHelper();
	auto BOAssistantHelper = breakoutRoomController->GetBOAssistantHelper();
	auto BOAttedeeHelper = breakoutRoomController->GetBOAttedeeHelper();
	auto BOCreatorHelper = breakoutRoomController->GetBOCreatorHelper();
	auto BODataHelper = breakoutRoomController->GetBODataHelper();

	//When simple user, all BO interfaces are NULL
#define NULLTOSTRING(x) x == nullptr ? wxT("NULL") :wxT("NOT NULL")

	AddLogEntry(wxString::Format(wxT("BKROOM: BOAdmin is %s"), NULLTOSTRING(BOAdmin)));
	AddLogEntry(wxString::Format(wxT("BKROOM: BOAssistantHelper is %s"), NULLTOSTRING(BOAssistantHelper)));
	AddLogEntry(wxString::Format(wxT("BKROOM: BOAttedeeHelper is %s"), NULLTOSTRING(BOAttedeeHelper)));
	AddLogEntry(wxString::Format(wxT("BKROOM: BOCreatorHelper is %s"), NULLTOSTRING(BOCreatorHelper)));
	AddLogEntry(wxString::Format(wxT("BKROOM: BODataHelper is %s\n"), NULLTOSTRING(BODataHelper)));

	/*
	* === I was assigned HOST
	* [+] BO_STATUS: BO_STATUS_EDIT
	* [+] BOAdmin is NOT NULL
	* [+] BOAssistantHelper is NOT NULL
	* [+] BOAttedeeHelper is NULL
	* [+] BOCreatorHelper is NOT NULL
	* [+] BODataHelper is NOT NULL
	*/

	//Determine number of rooms to create
	RoomMap roommap = GetRoomMap();

	//Create rooms
	for (auto& kv : roommap)
	{
		std::wstring roomname = std::format(L"Groupe#{0}", kv.first);
		const wchar_t* BO_ID = BOCreatorHelper->CreateBO(roomname.c_str());

		if (BO_ID == nullptr)
		{
			AddLogEntry(wxString::Format("BKROOM: Failed to create room: '%s'", roomname.c_str()), false);
			break;
		}
		else
		{
			AddLogEntry(wxString::Format("BKROOM: Created room: '%s'", roomname.c_str()));
		}

		//Set the room BO_ID
		kv.second = BO_ID;
		//There is a necessary time between calls
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	}

	//
	ZOOMSDK::BOOption option;
	option.countdown_seconds = ZOOMSDK::BO_STOP_COUNTDOWN::BO_STOP_NOT_COUNTDOWN;
	BOCreatorHelper->SetBOOption(option);

	ZOOMSDK::IList<const wchar_t*>* unassignedUserList = BODataHelper->GetUnassginedUserList();
	if (unassignedUserList == nullptr)
	{
		AddLogEntry(wxT("BKROOM: Failed GetUnassginedUserList"), false);
		return;
	}

	//Assign everyone to it's room
	for (int i = 0; i < unassignedUserList->GetCount(); i++)
	{
		const wchar_t* strUserID = unassignedUserList->GetItem(i);
		const wchar_t* pwUsername = BODataHelper->GetBOUserName(strUserID);

		//Now get a user struct
		MemoryUserDataManager::Instance()->FindUserMultiColumnSetVolatileUserId(
			pwUsername,
			0,
			[&](UserDataItem& data, bool flagFoundInAlias)
			{
				//Assign user to room
				const wchar_t* BO_ID = roommap[data.ugroup];
				if (BO_ID == nullptr)
				{
					return;
				}

				if (BOCreatorHelper->AssignUserToBO(strUserID, BO_ID))
				{
					AddLogEntry(wxString::Format(
						wxT("BKROOM: realname:'%s' WAS ASSIGNED to room"),
						pwUsername
					));
				}
				else
				{
					AddLogEntry(wxString::Format(
						wxT("BKROOM: Failed AssignUserToBO for realname:'%s'; WAS NOT assigned to room"),
						pwUsername
					), false);
				}
			}
		);

		std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	if (BOAdmin->StartBO())
	{
		AddLogEntry(wxT("BKROOM: Started rooms"));
	}
	else
	{
		AddLogEntry(wxT("BKROOM: Failed creating rooms"), false);
	}

	return;
}
void MainWindow::onSpotlightBtn_1_Click(wxCommandEvent&)
{
	std::thread thread_object([&]()
		{
			SpotlightUsername(grSpotlight_1_p1->GetValue().ToStdWstring());

			std::this_thread::sleep_for(std::chrono::milliseconds(2000));

			SpotlightUsername(grSpotlight_1_p2->GetValue().ToStdWstring());
		});
	thread_object.detach();
}
void MainWindow::onSpotlightBtn_2_Click(wxCommandEvent&)
{
	std::thread thread_object([&]()
		{
			SpotlightUsername(grSpotlight_2_p1->GetValue().ToStdWstring());

			std::this_thread::sleep_for(std::chrono::milliseconds(2000));

			SpotlightUsername(grSpotlight_2_p2->GetValue().ToStdWstring());
		});
	thread_object.detach();
}
void MainWindow::onSpotlightBtn_3_Click(wxCommandEvent&)
{
	std::thread thread_object([&]()
		{
			SpotlightUsername(grSpotlight_3_p1->GetValue().ToStdWstring());

			std::this_thread::sleep_for(std::chrono::milliseconds(2000));

			SpotlightUsername(grSpotlight_3_p2->GetValue().ToStdWstring());
		});
	thread_object.detach();
}
void MainWindow::SpotlightUsername(const std::wstring& username)
{
	if (!videoCtrl)
	{
		AddLogEntry(wxT("SPOTLIGHT: video controller is invalid"), false);
		return;
	}

	auto CallbackOnUserFound = [&](UserDataItem& data, bool flagFoundInAlias)
	{
		//This is in fact an issue because we call this function with an username
		//but all that we found is an alias
		if (flagFoundInAlias)
		{
			return;
		}
		if (ZOOMSDK::SDKError::SDKERR_SUCCESS == videoCtrl->SpotlightVideo(data.userId))
		{
			AddLogEntry(wxString::Format(
				wxT("SPOTLIGHT: added to spotlight realname:'%s' with uid:'%u'"),
				data.username.c_str(),
				data.userId
			));
		}
		else
		{
			AddLogEntry(wxString::Format(
				wxT("SPOTLIGHT: FAILED to add spotlight FOR realname:'%s' with uid:'%u'"),
				data.username.c_str(),
				data.userId
			));
		}
	};

	//
	if (!MemoryUserDataManager::Instance()->FindUserMultiColumnSetVolatileUserId(
		username,
		0,
		CallbackOnUserFound
	))
	{
		AddLogEntry(wxString::Format(
			wxT("SPOTLIGHT: name:'%s' with NOT FOUND"),
			username.c_str()
		), false);

		return;
	}
}

void MainWindow::onBtnSignIn_Click(wxCommandEvent& ev)
{
	SyncZoomConfigurationWithUIStatus();

	ZOOM_SDK_NAMESPACE::AuthContext authContext;
	ZOOM_SDK_NAMESPACE::InitParam initParam;
	initParam.strWebDomain = wxT("https://zoom.us");
	authContext.jwt_token = volatileData.zoomSdk.c_str();

	if (SDKInterfaceWrap::Instance().Init(initParam) != ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS)
	{
		AddLogEntry(wxT("Failed initializng the zoom sdk"), false);
		return;
	}
	else AddLogEntry(wxT("Zoom SDK was initialized"));

	// Init auth service if not already
	ZOOM_SDK_NAMESPACE::IAuthService* authService = SDKInterfaceWrap::Instance().GetAuthService();
	SDKInterfaceWrap::Instance().ListenInMeetingServiceMgrEvent(this);

	if (authService == NULL)
	{
		AddLogEntry(wxT("Failed getting a reference to the AuthService"), false);
		return;
	}
	else AddLogEntry(wxT("Got a reference to AuthService"));
	//
	if (authService->SDKAuth(authContext) != ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS)
	{
		AddLogEntry(wxT("Failed authorizing zoom sdk"), false);
		return;
	}
	else AddLogEntry(wxT("Zoom SDK succesfully authorized"));
}
void MainWindow::onBtnJoinMeeting_Click(wxCommandEvent& ev)
{
	//For the latest API changes , see https://marketplace.zoom.us/docs/sdk/native-sdks/windows/mastering-sdk/windows-sdk-functions/
	//There is another param struct apparently ZOOM_SDK_NAMESPACE::StartParam 

	SyncZoomConfigurationWithUIStatus();

	ZOOM_SDK_NAMESPACE::JoinParam joinParam;
	joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_NORMALUSER;
	joinParam.param.normaluserJoin.meetingNumber = std::stoull(volatileData.zoomRoomId);
	joinParam.param.normaluserJoin.psw = volatileData.zoomRoomPassword.c_str();
	joinParam.param.normaluserJoin.userName = volatileData.zoomUsername.c_str();
	joinParam.param.normaluserJoin.vanityID = NULL;
	joinParam.param.normaluserJoin.hDirectShareAppWnd = NULL;
	joinParam.param.normaluserJoin.customer_key = NULL;
	joinParam.param.normaluserJoin.webinarToken = NULL;
	joinParam.param.normaluserJoin.isVideoOff = false;
	joinParam.param.normaluserJoin.isAudioOff = false;
	joinParam.param.normaluserJoin.isDirectShareDesktop = false;

	meetingService = SDKInterfaceWrap::Instance().GetMeetingService();

	if (meetingService->Join(joinParam) != ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS)
	{
		AddLogEntry(wxT("Failed joining selected meeting room"), false);
		return;
	}
	else AddLogEntry(wxT("Joining room ... "));
	//
	if (NULL == (participantsController = SDKInterfaceWrap::Instance().GetMeetingParticipantsController()))
	{
		AddLogEntry(wxT("Failed getting a reference to the Participants Controller"), false);
		return;
	}
	else AddLogEntry(wxT("Got a reference to the Participants Controller"));
	//
	if (NULL == (waitingRoomController = SDKInterfaceWrap::Instance().GetMeetingWaitingRoomController()))
	{
		AddLogEntry(wxT("Failed getting a reference to the WaitingRoom Controller"), false);
		return;
	}
	else AddLogEntry(wxT("Got a reference to the WaitingRoom Controller"));
	//
	if (NULL == (chatController = SDKInterfaceWrap::Instance().GetMeetingChatController()))
	{
		AddLogEntry(wxT("Failed getting a reference to the Chat Controller"), false);
		return;
	}
	else AddLogEntry(wxT("Got a reference to the Chat Controller"));
	//
	if (NULL == (videoCtrl = SDKInterfaceWrap::Instance().GetMeetingVideoController()))
	{
		AddLogEntry(wxT("Failed getting a reference to the Video Controller"), false);
		return;
	}
	else AddLogEntry(wxT("Got a reference to the Video Controller"));
}

//IAuthServiceEvent
void MainWindow::onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret)
{
	if (ret != ZOOM_SDK_NAMESPACE::AuthResult::AUTHRET_SUCCESS)
	{
		AddLogEntry(wxT("Authentification failed"), false);
		return;
	}
	else AddLogEntry(wxT("Succesfully authentifcated"));

	ZOOM_SDK_NAMESPACE::LoginParam loginParam;
	loginParam.loginType = ZOOM_SDK_NAMESPACE::LoginType::LoginType_Email;
	loginParam.ut.emailLogin.userName = volatileData.zoomEmail.c_str();
	loginParam.ut.emailLogin.password = volatileData.zoomPassword.c_str();;
	loginParam.ut.emailLogin.bRememberMe = false;

	//Initiate login 
	if (ZOOM_SDK_NAMESPACE::SDKError::SDKERR_SUCCESS != SDKInterfaceWrap::Instance().GetAuthService()->Login(loginParam))
	{
		AddLogEntry(wxT("Failed to initialize login transaction"), false);
		return;
	}
	else AddLogEntry(wxT("Initiated login transaction"));

	//Get reference and initialize event listener for this interface.
	meetingService = SDKInterfaceWrap::Instance().GetMeetingService();
}
void MainWindow::onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS returnValue, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason)
{
	if (ZOOM_SDK_NAMESPACE::LOGINSTATUS::LOGIN_SUCCESS == returnValue)
	{
		AddLogEntry(wxT("Login was succesfull"));
	}
	else
	{
		AddLogEntry(wxT("Login failed"), false);
	}
}

//IMeetingServiceEvent
void MainWindow::onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int iResult)
{
	switch (status)
	{
	case ZOOM_SDK_NAMESPACE::MEETING_STATUS_RECONNECTING:
	case ZOOM_SDK_NAMESPACE::MEETING_STATUS_DISCONNECTING:
	case ZOOM_SDK_NAMESPACE::MEETING_STATUS_ENDED:
	case ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED:
		MemoryUserDataManager::Instance()->ForEach([](UserDataItem& data)
			{
				data.online = false;
			});

		listViewUsers->Refresh();
		break;
	}
}

//IMeetingParticipantsCtrlEvent
void MainWindow::onUserJoinCommon(UserDataItem& data, bool newEntry)
{
	if (newEntry)
	{
		data.Save();
		listViewUsers->AddUserDataMustRefreshManualyAfterwards(data);
	}

	data.online = true;
	listViewUsers->Refresh();
}
void MainWindow::onUserJoin(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList /*= NULL*/)
{
	SDKListHelper listHelper(lstUserID);

	auto pFilterClb = [&](ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo, unsigned int userId) -> bool
	{
		if (DoesUsernameContainSeparator(pUserInfo->GetUserNameW()))
		{
			AddLogEntry(wxString::Format(
				wxT("JOIN: entryname:'%s' with uid:'%u' contains a separator; to_expel=%s"),
				pUserInfo->GetUserNameW(),
				userId,
				volatileData.expelNamesWithSeparatorInside ? wxT("true") : wxT("false")
			), false);

			if (volatileData.expelNamesWithSeparatorInside)
			{
				participantsController->ExpelUser(userId);
			}
			return true; //Continue looping
		}

		auto CallbackOnUserFound = [&](UserDataItem& data, bool flagFoundInAlias)
		{
			if (!data.ban)
			{
				if (flagFoundInAlias)
				{
					AddLogEntry(wxString::Format(
						wxT("JOIN: entryname:'%s'(realname:%s) with uid:'%u'; will be renamed"),
						pUserInfo->GetUserNameW(),
						data.username.c_str(),
						userId
					)
					);

					//TODO: add checkbox
					participantsController->ChangeUserName(userId, data.username.c_str(), true);
				}
				else
				{
					AddLogEntry(wxString::Format(
						wxT("JOIN: entryname:'%s'(realname:%s) with uid:'%u'"),
						pUserInfo->GetUserNameW(),
						data.username.c_str(),
						userId)
					);
				}

				onUserJoinCommon(data, false);
			}
			else
			{
				AddLogEntry(wxString::Format(
					wxT("JOIN: entryname:'%s'(realname:%s) with uid:'%u' BANNED"),
					pUserInfo->GetUserNameW(),
					data.username.c_str(),
					userId)
				);

				participantsController->ExpelUser(userId);
			}
		};

		//
		if (!MemoryUserDataManager::Instance()->FindUserMultiColumnSetVolatileUserId(
			pUserInfo->GetUserNameW(),
			userId,
			CallbackOnUserFound
		))
		{
			UserDataItem data;
			data.admit = false;
			data.ban = false;
			data.stranger = true;
			data.username = pUserInfo->GetUserNameW();
			data.userId = userId;

			//Filter ';' from username
			std::erase(data.username, ';');

			AddLogEntry(wxString::Format(
				wxT("JOIN: entryname:'%s' with uid:'%u' joined but it's not in the database; ADDED TO DATABASE"),
				pUserInfo->GetUserNameW(),
				userId)
			);
			onUserJoinCommon(data, true);
		}

		return true; //keep looping
	};

	listHelper.ForEachItem(pFilterClb);
}
void MainWindow::onUserLeft(ZOOM_SDK_NAMESPACE::IList<unsigned int >* lstUserID, const wchar_t* strUserList /*= NULL*/)
{
	int count = lstUserID->GetCount();

	for (int i = 0; i < count; i++)
	{
		//Note: the user struct is already null by the time we get called
		unsigned int userId = lstUserID->GetItem(i);

		auto CallbackOnUserFound = [&](UserDataItem& data)
		{
			AddLogEntry(wxString::Format(wxT("LEFT: realname:'%s' with uid:'%u'"),
				data.username,
				data.userId)
			);

			data.online = false;
			data.userId = 0;

			listViewUsers->Refresh();
		};

		if (!MemoryUserDataManager::Instance()->FindUserByVolatileUserId(
			userId,
			CallbackOnUserFound
		))
		{
			AddLogEntry(wxString::Format(wxT("LEFT: UNKOWN user with uid:'%u'"), userId));
		}
	}
}
void MainWindow::onUserNameChanged(unsigned int userId, const wchar_t* userName)
{
	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo = participantsController->GetUserByUserID(userId);

	auto CallbackOnUserFound = [&](UserDataItem& data)
	{
		auto flagContainsSeparator = DoesUsernameContainSeparator(userName);
		AddLogEntry(wxString::Format(
			wxT("CHANGE: realname:'%s' with uid:'%u' wants to change username to '%s'; hasKeyword=%s"),
			data.username,
			data.userId,
			userName,
			flagContainsSeparator ? wxT("true") : wxT("false"))
		);

		if (flagContainsSeparator)
		{
			//TODO: if autorename to the old name

			return;
		}

		//If the new name is in fact the database name or is found inside the alias list.
		auto flagSameUsername = IsStringEqualToCaseInsensitive(data.username, userName); // ?????????????????????????????????
		auto flagUsernameIsInAlias = data.IsNameInAliasList(userName);
		if (flagSameUsername || flagUsernameIsInAlias)
		{
			return;
		}

		if (data.alias.size() == 0)
		{
			data.alias = userName;
		}
		else
		{
			data.alias += wxT(";");
			data.alias += std::wstring(userName);
		}

		data.Save();
		listViewUsers->Refresh();
	};

	if (!MemoryUserDataManager::Instance()->FindUserByVolatileUserId(userId, CallbackOnUserFound))
	{
		AddLogEntry(wxString::Format(
			wxT("CHANGE: user with uid:'%u' couldn't be found; newusername='%s'"),
			userId,
			userName)
		);
	}
}

//IMeetingWaitingRoomEvent
void MainWindow::onWatingRoomUserJoin(unsigned int userID)
{
	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfoWaitRoom = waitingRoomController->GetWaitingRoomUserInfoByID(userID);

	if (pUserInfoWaitRoom == NULL || pUserInfoWaitRoom->IsMySelf())
	{
		return;
	}

	//Find one user in the participants list that has the same username as the one that is in 
	//the waiting room right now.
	SDKListHelper listHelper(participantsController->GetParticipantsList());
	ZOOM_SDK_NAMESPACE::IUserInfo* pUserInfo = listHelper.GetUserInfoByName(pUserInfoWaitRoom->GetUserNameW());

	if (pUserInfo == NULL)
	{
		//Function called on user found in database
		auto CallbackOnUserFound = [&](UserDataItem& data, bool flagFoundInAlias)
		{
			if (!data.ban)
			{
				if (data.admit)
				{
					AddLogEntry(wxString::Format(
						wxT("WAITJOIN: entryname:'%s'(realname='%s') with uid:'%u' WAS IN WAITING ROOM -> TRANSFERED to meeting; "),
						pUserInfoWaitRoom->GetUserNameW(),
						data.username.c_str(),
						pUserInfoWaitRoom->GetUserID())
					);
					data.userId = pUserInfoWaitRoom->GetUserID();
					waitingRoomController->AdmitToMeeting(userID);
				}
				else
				{
					AddLogEntry(wxString::Format(
						wxT("WAITJOIN: entryname:'%s'(realname='%s') with uid:'%u' IS IN WAITING ROOM -> CANNOT BE AUTO-ADMITED(admit=false);"),
						pUserInfoWaitRoom->GetUserNameW(),
						data.username.c_str(),
						pUserInfoWaitRoom->GetUserID())
					);
				}
			}
			else
			{
				AddLogEntry(wxString::Format(
					wxT("WAITJOIN: entryname:'%s'(realname='%s') with uid:'%u' IS BANNED - NOT TRANSFERED; "),
					pUserInfoWaitRoom->GetUserNameW(),
					data.username.c_str(),
					pUserInfoWaitRoom->GetUserID())
				);
			}
		};

		//If user not found
		if (!MemoryUserDataManager::Instance()->FindUserMultiColumnSetVolatileUserId(
			pUserInfoWaitRoom->GetUserNameW(),
			0,
			CallbackOnUserFound
		))
		{
			AddLogEntry(wxString::Format(
				wxT("WAITJOIN: UNKOWN entryname:'%s' with uid:'%u' IS IN WAITING ROOM"),
				pUserInfoWaitRoom->GetUserNameW(),
				pUserInfoWaitRoom->GetUserID())
			);
		}
	}
	else
	{
		AddLogEntry(wxString::Format(
			wxT("WAITJOIN: entryname:'%s' with uid:'%u' -> THERE IS ANOTHER USER WITH SAME NAME ALREADY IN THE MEETING; left alone"),
			pUserInfoWaitRoom->GetUserNameW(),
			pUserInfoWaitRoom->GetUserID())
		);
	}
}
void MainWindow::onWatingRoomUserLeft(unsigned int userID) {}

//
bool MyApp::OnInit()
{
#ifdef __WXMSW__
	auto m_user32Dll = LoadLibrary(L"User32.dll");
	if (m_user32Dll)
	{
		SetProcessDPIAwareFunc pFunc = (SetProcessDPIAwareFunc)GetProcAddress(m_user32Dll, "SetProcessDPIAware");
		if (pFunc)
		{
			pFunc();
		}
		FreeLibrary(m_user32Dll);
		m_user32Dll = NULL;
	}
	// load the exception handler dll so we will get Dr MinGW at runtime
	LoadLibrary(wxT("exchndl.dll"));
#endif

	std::wstring cd = wxGetCwd().ToStdWstring();
	OutputDebugStringFmt(fmt::format(L"Current path is = {0}", cd));

	MainWindow* frame = new MainWindow(NULL, wxID_ANY, "Zoom Client - Native Attribute Manager", wxDefaultPosition, wxSize(550, 400));
	frame->Show(true);
	frame->Fit();
	return true;
}