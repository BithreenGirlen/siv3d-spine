#ifndef SIV3D_SPINE_PLAYER_H_
#define SIV3D_SPINE_PLAYER_H_

#include "spine_player.h"

class CSiv3dSpinePlayer : public CSpinePlayer
{
public:
	CSiv3dSpinePlayer();
	virtual ~CSiv3dSpinePlayer();

	bool loadSpineFromFile(const s3d::Array<s3d::FilePath>& atlasFilePaths, const s3d::Array<s3d::FilePath>& skeletonFilePaths);
	bool loadSpineFromMemory(const s3d::Array<s3d::Blob>& atlasFileData, const s3d::Array<s3d::FilePath>& textureDirectories, const s3d::Array<s3d::Blob>& skeletonFileData);

	bool addSpineFromFile(const s3d::FilePath& atlasFilePath, const s3d::FilePath& skelFilePath);

	void redraw();

	s3d::Mat3x2 calculateTransformMatrix(s3d::Size renderTargetSize = s3d::Graphics2D::GetRenderTargetSize()) const;
	s3d::Optional<s3d::Vector4D<float>> getCurrentBoundingOfSlot(const std::string& slotName) const;
private:
	void workOutDefaultScale() override;
	void workOutDefaultOffset() override;
};

#endif // !SIV3D_SPINE_PLAYER_H_
