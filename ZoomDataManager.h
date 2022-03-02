#pragma once

#include <vector>
#include <random>

//4db
#include <ctxt.h>

bool IsSubstringInString(std::wstring_view base, std::wstring_view subs);
bool IsStringEqualToCaseInsensitive(std::wstring_view str1, std::wstring_view str2);


class Database final {
private:
	Database() {};
public:
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;
	Database(Database&&) = delete;
	Database& operator=(Database&&) = delete;

	static Database* Instance() {
		static Database staticinstance;
		return &staticinstance;
	}

private:
	//Database context
	std::unique_ptr<fourdb::ctxt> context = std::make_unique<fourdb::ctxt>("ZoomLocalSqlDatabase.db");
public:
	fourdb::ctxt* GetContext() const;

public:
	struct IContextMethods
	{
		virtual bool Save() = 0;
		virtual void Delete() = 0;
	};
};

struct ZoomConfiguration : Database::IContextMethods
{
	static inline const std::wstring tableName = L"ZoomData";

	std::wstring zoomSdk;
	std::wstring zoomEmail;
	std::wstring zoomPassword;
	std::wstring zoomUsername;
	std::wstring zoomRoomId;
	std::wstring zoomRoomPassword;

	bool expelNamesWithSeparatorInside = false;
	bool allowAutoAdmit = false;

	bool Save() override;
	void Delete() override {}
	void Populate();
};

struct UserDataItem : Database::IContextMethods
{
	static inline const std::wstring tableName = L"UserData";
	static inline const std::wstring defaultImagePath = L"xitem.jpg";

	std::wstring primary;
	std::wstring imagePath = defaultImagePath;
	std::wstring username;
	std::wstring alias;

	bool admit = false;
	bool ban = false;
	bool stranger = true;

	unsigned int userId = 0;
	bool online = false;
	int ugroup = -1;
	int imageIndex = -1;

	bool Save() override;
	void Delete() override;
	
	bool IsNameInAliasList(std::wstring_view);
	static bool PopulateUserDataItemWithReaderResults(std::shared_ptr<fourdb::dbreader> reader, UserDataItem& oUserDataItem);
	static std::vector<UserDataItem> Read();
};


//
class MemoryUserDataManager final {
private:
	MemoryUserDataManager() {};
public:
	MemoryUserDataManager(const MemoryUserDataManager&) = delete;
	MemoryUserDataManager& operator=(const MemoryUserDataManager&) = delete;
	MemoryUserDataManager(MemoryUserDataManager&&) = delete;
	MemoryUserDataManager& operator=(MemoryUserDataManager&&) = delete;

	static MemoryUserDataManager* Instance() {
		static MemoryUserDataManager staticinstance;
		return &staticinstance;
	}

private:
	using UserVector = std::vector<UserDataItem>;
	std::unique_ptr<UserVector> store = std::make_unique<UserVector>();
	std::mutex g_mutex;
public:
	//Find user in the store. If user found then call the function callback with desired params.
	bool FindUserMultiColumnSetVolatileUserId(
		const std::wstring& name,
		unsigned int userId,
		const std::function<void(UserDataItem&, bool)>&
	);

	//Find user by id in the store. If user found then call the function callback.
	bool FindUserByVolatileUserId(
		unsigned int userId,
		const std::function<void(UserDataItem&)>&
	);

	void AddInStore(const UserDataItem& item, long& size);
	void EraseIf(const std::function<bool(const UserDataItem&)>& pred, long& size);
	void ForEach(const std::function<void(UserDataItem&)>& pred) noexcept;
	void Sort(const std::function<bool(const UserDataItem&, const UserDataItem&)>& pred);
	long GetStoreSize() noexcept;
	UserDataItem& GetAtUnsafe(long index) noexcept;
};
