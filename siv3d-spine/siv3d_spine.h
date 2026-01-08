#ifndef SIV3D_SPINE_H_
#define SIV3D_SPINE_H_

/* "s3v::color"と"spine::color"の衝突を避ける。 */
#define NO_S3D_USING
#include <Siv3D.hpp>

/* Spine 4.0 以前に於ける<MathUtils.h>と<Windows.h>内の衝突を避ける。 */
#if SIV3D_PLATFORM(WINDOWS)
#undef min
#undef max
#endif
#include <spine/spine.h>

class CS3dSpineDrawable
{
public:
	CS3dSpineDrawable(spine::SkeletonData* pSkeletonData, spine::AnimationStateData* pAnimationStateData = nullptr);
	~CS3dSpineDrawable();

	spine::Skeleton* skeleton = nullptr;
	spine::AnimationState* animationState = nullptr;

	/// @brief 乗算済みアルファ適用有無。Spine 3.8より古い場合のみ有効。4.0からはAtlasPageの同特性値を参照するため手動変更不可。
	void PremultiplyAlpha(bool toBePremultiplied);
	bool IsAlphaPremultiplied() const;

	/// @brief スロットの指定する混色法を無視して通常混色を適用するか否か。
	void ForceBlendModeNormal(bool toForce);
	bool IsBlendModeNormalForced() const;

	void Update(float fDelta);
	void Draw();

	/// @brief 描画対象から除外するスロットを設定
	void SetSlotsToLeaveOut(spine::Vector<spine::String>& slotNames);

	/// @brief 全体の境界矩形を算出
	s3d::Vector4D<float> GetBoundingBox() const;
	/// @brief 或るスロットの境界矩形を算出。
	s3d::Optional<s3d::Vector4D<float>> GetBoundingBoxOfSlot(const char* slotName, size_t nameLength) const;
private:
	bool m_hasOwnAnimationStateData = false;
	bool m_isAlphaPremultiplied = false;
	bool m_toForceBlendModeNormal = false;

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
