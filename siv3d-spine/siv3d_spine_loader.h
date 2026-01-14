#ifndef SIV3D_SPINE_LOADER_H_
#define SIV3D_SPINE_LOADER_H_

#include <Siv3D.hpp>

#include <spine/Atlas.h>
#include <spine/SkeletonData.h>

/* 一先ず、DefaultSpineExtensionによるファイル読み込みの廃止を目的とした暫定版。 */

/// @brief Spineファイルからの静的データ(Atlas並びSkeletonData)作成
namespace siv3d_spine_loader
{
	std::shared_ptr<spine::Atlas> ReadAtlasFromFile(const s3d::FilePath& filePath, spine::TextureLoader* pTextureLoader);
	std::shared_ptr<spine::Atlas> ReadAtlasFromMemory(const s3d::Array<s3d::Byte>& atlasFileData, const s3d::FilePath &textureDirectory, spine::TextureLoader* pTextureLoader);

	/* Todo: ファイル読み込みの場合、バイナリ形式かテキスト形式かは内部で判断して公開関数・引数の数を減らす。*/

	std::shared_ptr<spine::SkeletonData> ReadJsonSkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadJsonSkeletonFromMemory(const s3d::String& jsonSkeleton, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadJsonSkeletonFromMemory(const s3d::Array<s3d::Byte>& jsonSkeleton, spine::Atlas* pAtlas);

	std::shared_ptr<spine::SkeletonData> ReadBinarySkeletonFromFile(const s3d::FilePath& filePath, spine::Atlas* pAtlas);
	std::shared_ptr<spine::SkeletonData> ReadBinarySkeletonFromMemory(const s3d::Array<s3d::Byte>& binarySkeleton, spine::Atlas* pAtlas);
}
#endif // !SIV3D_SPINE_LOADER_H_
