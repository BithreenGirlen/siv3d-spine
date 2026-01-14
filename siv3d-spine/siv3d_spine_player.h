#ifndef SIV3D_SPINE_PLAYER_H_
#define SIV3D_SPINE_PLAYER_H_

#include "spine_player.h"

class CSiv3dSpinePlayer : public CSpinePlayer
{
public:
	CSiv3dSpinePlayer();
	virtual ~CSiv3dSpinePlayer();

	/* ファイル読み込み方法の改修中に就き一次的に設計が終わってるが、DefaultSpineExtensionを排除しての動作を確認でき次第整理予定。*/

	bool loadSpineFromFile(const s3d::Array<s3d::String>& atlasFilePaths, const s3d::Array<s3d::String>& skeletonFilePaths, bool isBinarySkel);
	bool loadSpineFromMemory(const s3d::Array<s3d::Array<s3d::Byte>>& atlasFileData, const s3d::Array<s3d::String>& textureDirectories, const s3d::Array<s3d::Array<s3d::Byte>>& skeletonFileData, bool isBinarySkel);

	bool addSpineFromFile(const s3d::String& atlasFilePath, const s3d::String& skelFilePath, bool isBinarySkel);

	void redraw();

	s3d::Mat3x2 calculateTransformMatrix(s3d::Size renderTargetSize = s3d::Graphics2D::GetRenderTargetSize()) const;
	s3d::Optional<s3d::Vector4D<float>> getCurrentBoundingOfSlot(const std::string& slotName) const;
private:
	void workOutDefaultScale() override;
	void workOutDefaultOffset() override;
};

#endif // !SIV3D_SPINE_PLAYER_H_
