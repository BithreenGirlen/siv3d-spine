#ifndef SIV3D_SPINE_LOADER_H_
#define SIV3D_SPINE_LOADER_H_

#include <Siv3D.hpp>

#include <spine/Atlas.h>
#include <spine/SkeletonData.h>

/// @brief Spine出力ファイルからの静的データ(Atlas並びSkeletonData)作成
namespace siv3d_spine_loader
{
	/// @brief Atlas作成
	std::shared_ptr<spine::Atlas> ReadAtlasFromFile(const s3d::FilePath& filePath, spine::TextureLoader* pTextureLoader);
	std::shared_ptr<spine::Atlas> ReadAtlasFromMemory(const s3d::Blob& atlasFileData, const s3d::FilePath &textureDirectory, spine::TextureLoader* pTextureLoader);

	/// @brief 内部でJSON形式かバイナリ形式か判断してSkeletonData作成。
	/// @remark 明示的に指定した方が高速です。
	std::shared_ptr<spine::SkeletonData> ReadSkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadSkeletonFromMemory(const s3d::Blob& skeletonFileData, spine::Atlas* pAtlas);

	/// @brief JSON形式からSkeletonData作成。
	std::shared_ptr<spine::SkeletonData> ReadJsonSkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadJsonSkeletonFromMemory(const s3d::String& jsonSkeleton, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadJsonSkeletonFromMemory(const s3d::Blob& jsonSkeleton, spine::Atlas* pAtlas);

	/// @brief バイナリ形式からSkeletonData作成。
	std::shared_ptr<spine::SkeletonData> ReadBinarySkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadBinarySkeletonFromMemory(const s3d::Blob& binarySkeleton, spine::Atlas* pAtlas);
}
#endif // !SIV3D_SPINE_LOADER_H_
