#ifndef SIV3D_SPINE_H_
#define SIV3D_SPINE_H_

/* "s3d::color"と"spine::color"の衝突を避ける。 */
#define NO_S3D_USING
#include <Siv3D.hpp>

/* Spine 4.0 以前に於ける<MathUtils.h>と<Windows.h>内の定義の衝突を避ける。 */
#if SIV3D_PLATFORM(WINDOWS)
#undef min
#undef max
#endif
#include <spine/spine.h>

class CS3dSpineDrawable
{
public:
	CS3dSpineDrawable(spine::SkeletonData* pSkeletonData);
	~CS3dSpineDrawable();

	spine::Skeleton* skeleton() const;
	spine::AnimationState* animationState() const;

	/// @brief 乗算済みアルファ適用有無。Spine 3.8より古い場合のみ有効で、4.0からはAtlasPageの同値を参照するため手動変更不可
	void premultiplyAlpha(bool toBePremultiplied);
	bool isAlphaPremultiplied() const;

	/// @brief スロット指定の混色法を無視して通常混色法を適用するか否か
	void forceBlendModeNormal(bool toForce);
	bool isBlendModeNormalForced() const;

	void update(float fDelta);
	void draw();

	/// @brief 描画対象から除外するスロットを設定
	void setSlotsToLeaveOut(spine::Vector<spine::String>& slotNames);

	/// @brief 全体の境界矩形を算出
	s3d::Vector4D<float> getBoundingBox() const;
	/// @brief スロットの境界矩形を算出
	s3d::Optional<s3d::Vector4D<float>> getBoundingBoxOfSlot(const char* slotName, size_t nameLength) const;
private:
	bool m_isAlphaPremultiplied = false;
	bool m_toForceBlendModeNormal = false;

	spine::Skeleton* m_skeleton = nullptr;
	spine::AnimationState* m_animationState = nullptr;

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

	void load(spine::AtlasPage& page, const spine::String& path) override;
	void unload(void* texture) override;
};

#endif // SIV3D_SPINE_H_
