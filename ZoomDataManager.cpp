#include "ZoomDataManager.h"
#include "stdafx.h"



bool IsSubstringInString(std::wstring_view base, std::wstring_view subs)
{
	auto it = std::search(
		std::begin(base), std::end(base), std::begin(subs), std::end(subs), [](wchar_t ch1, wchar_t ch2)
		{
			return std::towupper(ch1) == std::towupper(ch2);
		}
	);

	return (it != std::end(base));
}
bool IsStringEqualToCaseInsensitive(std::wstring_view str1, std::wstring_view str2)
{
	return (str1.size() == str2.size()) && IsSubstringInString(str1, str2);
}
std::wstring GeneratePrimaryKey()
{
	auto device = std::random_device{};
	auto generate_len = 16;

	std::mt19937 generator{ device() };
	std::uniform_int_distribution<int> distribution{ 'a', 'z' };
	std::wstring rand_str(generate_len, '\0');

	for (auto& dis : rand_str)
	{
		dis = distribution(generator);
	}
	return rand_str;
}


fourdb::ctxt* Database::GetContext() const
{
	return context.get();
}

//ZoomData
bool ZoomConfiguration::Save()
{
	//Delete previous data
	auto context = Database::Instance()->GetContext();
	context->drop(tableName);

	fourdb::strnum primaryKey = GeneratePrimaryKey();
	fourdb::paramap columnData
	{
		{ L"zoomSdk", zoomSdk },
		{ L"zoomEmail", zoomEmail },
		{ L"zoomPassword", zoomPassword },
		{ L"zoomUsername", zoomUsername },
		{ L"zoomRoomId", zoomRoomId },
		{ L"zoomRoomPassword", zoomRoomPassword },
		{ L"ExpelNamesWithSeparatorInside", expelNamesWithSeparatorInside },
		{ L"AllowAutoAdmit", allowAutoAdmit }
	};
	context->define(tableName, primaryKey, columnData);
	return true;
}
void ZoomConfiguration::Populate()
{
	static auto queryString = L"SELECT zoomSdk, zoomEmail, zoomPassword, zoomUsername, zoomRoomId, zoomRoomPassword, expelNamesWithSeparatorInside, allowAutoAdmit FROM " + tableName;
	auto select = fourdb::sql::parse(queryString);

	auto reader = Database::Instance()->GetContext()->execQuery(select);
	if (reader->read())
	{
		zoomSdk = reader->getString(0);
		zoomEmail = reader->getString(1);
		zoomPassword = reader->getString(2);
		zoomUsername = reader->getString(3);
		zoomRoomId = reader->getString(4);
		zoomRoomPassword = reader->getString(5);
		expelNamesWithSeparatorInside = static_cast<bool>(reader->getDouble(6));
		allowAutoAdmit = static_cast<bool>(reader->getDouble(7));
	}
	else
	{
		zoomSdk = L"";
		zoomEmail = L"";
		zoomPassword = L"";
		zoomUsername = L"HelperOnline";
		zoomRoomId = L"";
		zoomRoomPassword = L"";
		expelNamesWithSeparatorInside = false;
		allowAutoAdmit = false;
	}
}


//
bool UserDataItem::IsNameInAliasList(std::wstring_view name)
{
	//ranges::views::split returns a range of ranges
	auto ranges = alias | std::ranges::views::split(L';');
	for (const auto& range : ranges)
	{
		auto word = std::wstring_view(&*range.begin(), std::ranges::distance(range));
		if (IsStringEqualToCaseInsensitive(word, name))
		{
			return true;
		}
	}
	return false;
}
bool UserDataItem::PopulateUserDataItemWithReaderResults(std::shared_ptr<fourdb::dbreader> reader, UserDataItem& oUserDataItem)
{
	if (reader->read() == false)
	{
		return false;
	}

	oUserDataItem.primary = reader->getString(0);
	oUserDataItem.imagePath = reader->getString(1);
	oUserDataItem.username = reader->getString(2);
	oUserDataItem.alias = reader->getString(3);
	oUserDataItem.admit = static_cast<bool>(reader->getDouble(4));
	oUserDataItem.ban = static_cast<bool>(reader->getDouble(5));
	oUserDataItem.stranger = static_cast<bool>(reader->getDouble(6));
	oUserDataItem.ugroup = static_cast<int>(reader->getDouble(7));

	if (oUserDataItem.imagePath.size() == 0)
	{
		oUserDataItem.imagePath = defaultImagePath;
	}

	return true;
}
bool UserDataItem::Save()
{
	if (username.size() == 0)
	{
		return false;
	}

	//Delete previous data
	auto context = Database::Instance()->GetContext();
	Delete();

	if (primary.size() == 0)
	{
		primary = GeneratePrimaryKey();
	}

	fourdb::paramap columnData
	{
		{ L"imagePath", imagePath },
		{ L"username", username },
		{ L"alias", alias },
		{ L"admit", admit },
		{ L"ban", ban },
		{ L"stranger", stranger },
		{ L"ugroup", ugroup }
	};
	context->define(tableName, primary, columnData);
	return true;
}
void UserDataItem::Delete()
{
	if (primary.size() > 0)
	{
		Database::Instance()->GetContext()->deleteRow(tableName, primary);
	}
}
std::vector<UserDataItem> UserDataItem::Read()
{
	static auto queryString = L"SELECT value, imagePath, username, alias, admit, ban, stranger, ugroup FROM " + tableName;
	std::vector<UserDataItem> collection;
	collection.reserve(25);

	auto select = fourdb::sql::parse(queryString);
	auto reader = Database::Instance()->GetContext()->execQuery(select);

	while (true)
	{
		UserDataItem item;
		bool flagReadEntry = PopulateUserDataItemWithReaderResults(reader, item);

		if (!flagReadEntry)
		{
			break;
		}

		collection.push_back(item);
	}

	return collection;
}

bool MemoryUserDataManager::FindUserMultiColumnSetVolatileUserId(
	const std::wstring& name,
	unsigned int userId,
	const std::function<void(UserDataItem&, bool)>& callback
)
{
	const std::lock_guard<std::mutex> lock(g_mutex);

	//I will not allow searching a username in the database containing 
	//	the separator.
	if (name.find(';') != std::wstring::npos)
	{
		return false;
	}

	UserDataItem* pitem = nullptr;
	bool foundInAlias = false;

	//Search first in the entire column of usernames.
	//If entry not found then search in the alias column.
	//Is found in an alias list, consider only the earliest entries.
	for (UserDataItem& item : *store)
	{
		if (IsStringEqualToCaseInsensitive(item.username, name))
		{
			pitem = &item;
			break;
		}
		else if ((foundInAlias == false) && item.IsNameInAliasList(name))
		{
			//Store only the first reference.
			//But don't stop here because we still have to finish looping through all the usernames.
			pitem = &item;
			foundInAlias = true;
		}
	}

	if (pitem == nullptr)
	{
		return false;
	}

	if (userId > 0)
	{
		pitem->userId = userId;
	}
	callback(*pitem, foundInAlias);

	return true;
}
bool MemoryUserDataManager::FindUserByVolatileUserId(
	unsigned int userId,
	const std::function<void(UserDataItem&)>& callback
)
{
	const std::lock_guard<std::mutex> lock(g_mutex);

	for (bool found = false; UserDataItem & item : *store)
	{
		if (item.userId == userId)
		{
			callback(item);
			return true;
		}
	}

	return false;
}
void MemoryUserDataManager::AddInStore(const UserDataItem& item, long& size)
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	store->push_back(item);
	size = store->size();
}
void MemoryUserDataManager::EraseIf(const std::function<bool(const UserDataItem&)>& pred, long& size)
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	const auto& it = std::remove_if(store->begin(), store->end(), pred);
	store->erase(it, store->end());
	size = store->size();
}
void MemoryUserDataManager::ForEach(const std::function<void(UserDataItem&)>& pred) noexcept
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	std::for_each(store->begin(), store->end(), pred);
}
void MemoryUserDataManager::Sort(const std::function<bool(const UserDataItem&, const UserDataItem&)>& pred)
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	std::sort(store->begin(), store->end(), pred);
}
long MemoryUserDataManager::GetStoreSize() noexcept
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	return store->size();
}
UserDataItem& MemoryUserDataManager::GetAtUnsafe(long index) noexcept
{
	const std::lock_guard<std::mutex> lock(g_mutex);
	return (*store)[index];
}