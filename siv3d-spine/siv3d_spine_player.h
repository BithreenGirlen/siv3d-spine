#ifndef SIV3D_SPINE_PLAYER_H_
#define SIV3D_SPINE_PLAYER_H_

#include "spine_player.h"

class CSiv3dSpinePlayer : public CSpinePlayer
{
public:
	CSiv3dSpinePlayer();
	virtual ~CSiv3dSpinePlayer();

	void Redraw();

	void OnResize(const s3d::Size& size) { m_sceneSize = size; }

	s3d::Mat3x2 CalculateTransformMatrix() const;
	s3d::Optional<s3d::Vector4D<float>> GetCurrentBoundingOfSlot(const std::string& slotName) const;
private:
	void WorkOutDefaultScale() override;
	void WorkOutDefaultOffset() override;

	s3d::Size m_sceneSize;
};

#endif // !SIV3D_SPINE_PLAYER_H_
