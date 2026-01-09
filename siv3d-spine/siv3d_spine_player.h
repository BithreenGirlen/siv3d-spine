#ifndef SIV3D_SPINE_PLAYER_H_
#define SIV3D_SPINE_PLAYER_H_

#include "spine_player.h"

class CSiv3dSpinePlayer : public CSpinePlayer
{
public:
	CSiv3dSpinePlayer();
	virtual ~CSiv3dSpinePlayer();

	void redraw();

	void onResize(const s3d::Size& size) { m_sceneSize = size; }

	s3d::Mat3x2 calculateTransformMatrix() const;
	s3d::Optional<s3d::Vector4D<float>> getCurrentBoundingOfSlot(const std::string& slotName) const;
private:
	void workOutDefaultScale() override;
	void workOutDefaultOffset() override;

	s3d::Size m_sceneSize;
};

#endif // !SIV3D_SPINE_PLAYER_H_
