

#include <spine/SkeletonJson.h>
#include <spine/SkeletonBinary.h>

#include "siv3d_spine_loader.h"


namespace siv3d_spine_loader
{
	/*
	* バイナリ形式格納データ型式仕様
	* |  型式  |  長さ(Bytes)  |
	* | variant | 1- 5 |
	* | int | 4 |
	* | float | 4 |
	* | bool | 1 | 1がtrue, 0がfalse |
	* | string | variant + n | [Variant - 1]の値がバイナリ長を示し、UTF-8で表現された文字列が続く。空文字はVarinat = 1で表現。 |
	*
	* バイナリ形式ヘッダ仕様
	* |  型式  |  内容  |
	* | string | ハッシュ | // Spine 4.0, 4.1ではstringではなくint型2つから成る16進数表記
	* | string | 版 |
	* | float | 開始位置x座標 | // Spine 3.7以前には存在せず、この位置から幅データが始まる。 |
	* | float | 開始位置y座標 | // Spine 3.7以前には存在しない。 |
	* | float | 幅 |
	* | float | 高さ |
	* | bool | 任意データ有無 |
	* | string | 存在する場合、各種任意データ。 |
	*
	* ヘッダ直後にBoneの記述が続く。この並びはSpine 2.1より4.2に至るまで変更なし。
	*
	*/

	/// @brief Skeletonファイル形式
	enum class SkeletonFormat
	{
		Neither, /* Spine出力ファイルではないと判断 */
		Json,
		Binary,
	};

	/// @brief JSON形式と思われるか
	static bool IsLikelyJsonSkeleton(const s3d::Blob& blob)
	{
		/*
		* 空白文字なしで考えると、
		* ヘッダ開始 : '{' + "skeleton" + ':' + '{' = 12
		* ハッシュ組 : "hash" + ':' + '"' + 高々32バイト + '"' + ',' = 42
		* 版組 : "spine" + ':' + '"' + 高々6バイト + '"' + ',' = 17
		* 12 + 42 + 17 = 81を空白文字" \r\n\t"を考慮して凡そ2倍にしておく。
		* 実際にはBoneの記述が続くのでこれよりは長い筈。
		*/
		static constexpr size_t MinJsonSkeletonFileSize = 160;
		if (blob.size() < MinJsonSkeletonFileSize)return false;

		static constexpr const unsigned char skeleton[] = R"("skeleton")";
		static constexpr const unsigned char version[] = R"("spine")";

		const unsigned char* begin = reinterpret_cast<const unsigned char*>(blob.data());
		const unsigned char* end = begin + MinJsonSkeletonFileSize;
		const auto& skelIter = std::search(begin, end, skeleton, skeleton + sizeof(skeleton) - 1);
		const auto& versionIter = std::search(begin, end, version, version + sizeof(version) - 1);

		return (skelIter != end) && (versionIter != end);
	}

	/// @brief ハッシュ文字列か
	static bool IsHashString(std::string_view s)
	{
		return std::ranges::all_of(s, [](const char& c) {return std::isalnum(static_cast<const unsigned char>(c)) != 0 || c == '+' || c == '/'; });
	}

	/// @brief 版文字列か
	static bool IsVersionString(std::string_view s)
	{
		return std::ranges::all_of(s, [](const char& c) {return std::isdigit(static_cast<const unsigned char>(c)) != 0 || c == '.'; });
	}

	/// @brief 16進数表記ハッシュと版文字列で始まるか
	static bool StartsWithHexHashPrecedingVersion(const s3d::Blob& blob)
	{
		static constexpr const size_t HashLength = 8;
		/* 8 + 1 + 6 */
		static constexpr const size_t MinHashStringSize = HashLength + 7;
		if (blob.size() < MinHashStringSize)return false;

		s3d::MemoryViewReader memoryViewReader(blob.data(), MinHashStringSize);
		memoryViewReader.skip(HashLength);

		s3d::uint8 length;
		[[maybe_unused]] s3d::uint64 read = memoryViewReader.read(&length, sizeof(length));
		if (length < 1)return false;
		if (--length; blob.size() < static_cast<size_t>(memoryViewReader.getPos() + length))return false;

		std::string_view version(reinterpret_cast<const std::string_view::value_type*>(&blob[memoryViewReader.getPos()]), length);

		return IsVersionString(version);
	}

	/// @brief 文字列表記のハッシュと版文字列で始まるか
	static bool StartsWithStringHashPrecedingVersion(const s3d::Blob& blob)
	{
		/*
		* ハッシュは高々32バイト(上位最大16 + 下位最大16)
		* 版は高々6バイト(X.X.XX)
		* 従って 1 + 16 + 16 + 1 + 6 = 40が版までの最大長。
		* 実際にはBoneの記述が続くのでこれよりは長い筈。
		*/
		static constexpr const size_t MinBinarySkeletonFileSize = 40;
		if (blob.size() < MinBinarySkeletonFileSize) return false;

		s3d::MemoryViewReader memoryViewReader(blob.data(), blob.size());

		s3d::uint8 length;
		s3d::uint64 read = memoryViewReader.read(&length, sizeof(length));
		if (length < 1)return false;
		if (--length; blob.size() < static_cast<size_t>(memoryViewReader.getPos() + length))return false;

		std::string_view hash(reinterpret_cast<const std::string_view::value_type*>(&blob[memoryViewReader.getPos()]), length);

		memoryViewReader.skip(length);
		read = memoryViewReader.read(&length, sizeof(length));
		if (length < 1)return false;
		if (--length; blob.size() < static_cast<size_t>(memoryViewReader.getPos() + length))return false;

		std::string_view version(reinterpret_cast<const std::string_view::value_type*>(&blob[memoryViewReader.getPos()]), length);

		return IsHashString(hash) && IsVersionString(version);
	}

	/// @brief Skeletonファイルの出力形式を推測
	static SkeletonFormat GuessSkeletonFormat(const s3d::Blob& blob)
	{
		if (IsLikelyJsonSkeleton(blob))return SkeletonFormat::Json;
		if (StartsWithHexHashPrecedingVersion(blob))return SkeletonFormat::Binary; /* Spine 4.0と4.1のバイナリ形式 */
		if (StartsWithStringHashPrecedingVersion(blob))return SkeletonFormat::Binary; /* 上記以外の版のバイナリ形式 */

		return SkeletonFormat::Neither;
	}
}


std::shared_ptr<spine::Atlas> siv3d_spine_loader::ReadAtlasFromFile(const s3d::FilePath& filePath, spine::TextureLoader* pTextureLoader)
{
	s3d::Blob blob;
	bool result = blob.createFromFile(filePath);
	if (!result) return nullptr;

	const s3d::FilePath textutreDirectory = s3d::FileSystem::IsResourcePath(filePath) ?
		[&filePath]() ->const s3d::FilePath
		{
			size_t pos = filePath.lastIndexOf(U'/');
			if (pos == s3d::FilePath::npos)pos = 0;
			return filePath.substr(0, pos);
		}() :
		s3d::FileSystem::ParentPath(filePath);
	if (textutreDirectory.empty()) return nullptr;

	return ReadAtlasFromMemory(blob, textutreDirectory, pTextureLoader);
}

std::shared_ptr<spine::Atlas> siv3d_spine_loader::ReadAtlasFromMemory(const s3d::Blob& atlasFileData, const s3d::FilePath& textureDirectory, spine::TextureLoader* pTextureLoader)
{
	const std::string u8TextureDirectory = s3d::Unicode::ToUTF8(textureDirectory);

	return std::make_shared<spine::Atlas>(reinterpret_cast<const char*>(atlasFileData.data()), static_cast<int>(atlasFileData.size()), u8TextureDirectory.c_str(), pTextureLoader);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadSkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas)
{
	s3d::Blob blob;
	bool result = blob.createFromFile(filePath);
	if (!result) return nullptr;

	return ReadSkeletonFromMemory(blob, pAtlas);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadSkeletonFromMemory(const s3d::Blob& skeletonFileData, spine::Atlas* pAtlas)
{
	SkeletonFormat skeletonFormat = GuessSkeletonFormat(skeletonFileData);
	switch (skeletonFormat)
	{
	case SkeletonFormat::Json:
		return ReadJsonSkeletonFromMemory(skeletonFileData, pAtlas);
	case SkeletonFormat::Binary:
		return ReadBinarySkeletonFromMemory(skeletonFileData, pAtlas);
	default: break;
	}

	return nullptr;
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadJsonSkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas)
{
	s3d::TextReader textReader;
	bool result = textReader.open(filePath);
	if (!result)return nullptr;

	s3d::String jsonSkeleton = textReader.readAll();
	if (jsonSkeleton.empty())return nullptr;

	return ReadJsonSkeletonFromMemory(jsonSkeleton, pAtlas);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadJsonSkeletonFromMemory(const s3d::String& jsonSkeleton, spine::Atlas* pAtlas)
{
	const std::string u8JsonSkeleton = s3d::Unicode::ToUTF8(jsonSkeleton);

	spine::SkeletonJson jsonSkeletonParser(pAtlas);
	jsonSkeletonParser.setScale(1.f);

	spine::SkeletonData* pSkeletonData = jsonSkeletonParser.readSkeletonData(u8JsonSkeleton.c_str());
	if (pSkeletonData == nullptr)
	{
		const s3d::String error = s3d::Unicode::FromUTF8(jsonSkeletonParser.getError().buffer());
		s3d::Logger << error;

		return nullptr;
	}

	return std::shared_ptr<spine::SkeletonData>(pSkeletonData);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadJsonSkeletonFromMemory(const s3d::Blob& jsonSkeleton, spine::Atlas* pAtlas)
{
	spine::SkeletonJson jsonSkeletonParser(pAtlas);
	jsonSkeletonParser.setScale(1.f);

	spine::SkeletonData* pSkeletonData = jsonSkeletonParser.readSkeletonData(reinterpret_cast<const char*>(jsonSkeleton.data()));
	if (pSkeletonData == nullptr)
	{
		const s3d::String error = s3d::Unicode::FromUTF8(jsonSkeletonParser.getError().buffer());
		s3d::Logger << error;

		return nullptr;
	}

	return std::shared_ptr<spine::SkeletonData>(pSkeletonData);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadBinarySkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas)
{
	s3d::Blob blob;
	bool result = blob.createFromFile(filePath);
	if (!result) return nullptr;

	return ReadBinarySkeletonFromMemory(blob, pAtlas);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadBinarySkeletonFromMemory(const s3d::Blob& binarySkeleton, spine::Atlas* pAtlas)
{
	spine::SkeletonBinary binarySkeletonParser(pAtlas);
	binarySkeletonParser.setScale(1.f);

	spine::SkeletonData* pSkeletonData = binarySkeletonParser.readSkeletonData(reinterpret_cast<const unsigned char*>(binarySkeleton.data()), static_cast<int>(binarySkeleton.size()));
	if (pSkeletonData == nullptr)
	{
		const s3d::String error = s3d::Unicode::FromUTF8(binarySkeletonParser.getError().buffer());
		s3d::Logger << error;

		return nullptr;
	}

	return std::shared_ptr<spine::SkeletonData>(pSkeletonData);
}
