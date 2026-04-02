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
	/// @brief 描画対象の寸法に合うよう変形する行列を算出。s3d::Transformer2Dと組み合わせることを想定
	s3d::Mat3x2 calculateTransformMatrix(const s3d::Size&& renderTargetSize = s3d::Graphics2D::GetRenderTargetSize()) const;
	s3d::Optional<s3d::Vector4D<float>> getCurrentBoundingOfSlot(std::string_view slotName) const;
private:
	/// @brief モニタ解像度より大きい場合、収まるよう縮小率を算出。元々収まるなら等倍
	void workOutDefaultScale() override;
	/// @brief 最も原点に近い全体境界矩形の位置を算出
	void workOutDefaultOffset() override;
};

#endif // !SIV3D_SPINE_PLAYER_H_
