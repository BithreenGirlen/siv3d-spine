

#include <spine/SkeletonJson.h>
#include <spine/SkeletonBinary.h>

#include "siv3d_spine_loader.h"


std::shared_ptr<spine::Atlas> siv3d_spine_loader::ReadAtlasFromFile(const s3d::FilePath& filePath, spine::TextureLoader* pTextureLoader)
{
	s3d::Blob blob;
	bool result = blob.createFromFile(filePath);
	if (!result) return nullptr;

	const s3d::FilePath textutreDirectory = s3d::FileSystem::ParentPath(filePath);
	if (textutreDirectory.empty()) return nullptr;

    return ReadAtlasFromMemory(blob.asArray(), textutreDirectory, pTextureLoader);
}

std::shared_ptr<spine::Atlas> siv3d_spine_loader::ReadAtlasFromMemory(const s3d::Array<s3d::Byte>& atlasFileData, const s3d::FilePath& textureDirectory, spine::TextureLoader* pTextureLoader)
{
	const std::string u8TextureDirectory = s3d::Unicode::ToUTF8(textureDirectory);

	return std::make_shared<spine::Atlas>(reinterpret_cast<const char*>(atlasFileData.data()), static_cast<int>(atlasFileData.size()), u8TextureDirectory.c_str(), pTextureLoader);
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

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadJsonSkeletonFromMemory(const s3d::Array<s3d::Byte>& jsonSkeleton, spine::Atlas* pAtlas)
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

	return ReadBinarySkeletonFromMemory(blob.asArray(), pAtlas);
}

std::shared_ptr<spine::SkeletonData> siv3d_spine_loader::ReadBinarySkeletonFromMemory(const s3d::Array<s3d::Byte>& binarySkeleton, spine::Atlas* pAtlas)
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
