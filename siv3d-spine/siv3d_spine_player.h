#ifndef SIV3D_SPINE_PLAYER_H_
#define SIV3D_SPINE_PLAYER_H_

#include "spine_player.h"

class CSiv3dSpinePlayer : public CSpinePlayer
{
public:
	CSiv3dSpinePlayer();
	virtual ~CSiv3dSpinePlayer();

	void redraw();

	s3d::Mat3x2 calculateTransformMatrix(s3d::Size renderTargetSize = s3d::Graphics2D::GetRenderTargetSize()) const;
	s3d::Optional<s3d::Vector4D<float>> getCurrentBoundingOfSlot(const std::string& slotName) const;
private:
	void workOutDefaultScale() override;
	void workOutDefaultOffset() override;
};

#endif // !SIV3D_SPINE_PLAYER_H_
