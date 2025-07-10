#ifndef SIV3D_SPINE_H_
#define SIV3D_SPINE_H_

/* Avoid conflict between <MathUtils.h> and <Windows.h> */
#undef min
#undef max
#include <spine/spine.h>

/* Avoid ambiguity between s3v::color and spine::color */
#define NO_S3D_USING
#include <Siv3D.hpp>

class CS3dSpineDrawable
{
public:
	CS3dSpineDrawable(spine::SkeletonData* pSkeletonData, spine::AnimationStateData* pAnimationStateData = nullptr);
	~CS3dSpineDrawable();

	spine::Skeleton* skeleton = nullptr;
	spine::AnimationState* animationState = nullptr;

	/// @brief Whether alpha is premultiplied or not. For Spine 4.0 and later, this property is exported with atlas file,
	///	       but for Spine 3.8, should be configured based on other means.
	bool isAlphaPremultiplied = false;
	bool isToForceBlendModeNormal = false;

	void Update(float fDelta);
	void Draw();

	/// @brief Set slots to be excluded from rendering
	void SetSlotsToLeaveOut(spine::Vector<spine::String>& slotNames);

	s3d::Vector4D<float> GetBoundingBox() const;
private:
	bool m_hasOwnAnimationStateData = false;

	spine::Vector<float> m_worldVertices;

	s3d::Buffer2D m_buffer2d;

	spine::Vector<unsigned short> m_quadIndices;

	spine::SkeletonClipping m_skeletonClipping;

	spine::Vector<spine::String> m_slotsToLeaveOut;

	bool IsSlotToBeLeftOut(const spine::String& slotName);
};

class CS3dTextureLoader : public spine::TextureLoader
{
public:
	CS3dTextureLoader() {};
	virtual ~CS3dTextureLoader() {};

	virtual void load(spine::AtlasPage& page, const spine::String& path);
	virtual void unload(void* texture);
};

#endif // SIV3D_SPINE_H_
