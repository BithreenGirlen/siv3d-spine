#ifndef SPINE_PLAYER_H_
#define SPINE_PLAYER_H_

/* Base-type spine player regardless of rendering library. */

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "siv3d_spine.h"
using FPoint2 = s3d::Vector2D<float>;
using CSpineDrawable = CS3dSpineDrawable;
using CTextureLoader = CS3dTextureLoader;


class CSpinePlayer
{
public:
	CSpinePlayer();
	virtual ~CSpinePlayer();

	/// @brief 位置・速度・尺度・動作名を同期して再生するSpineの数を取得
	size_t getNumberOfSpines() const;
	bool hasSpineBeenLoaded() const;

	void update(float fDelta);

	/// @brief 速度・尺度・位置初期化
	void resetScale();

	/// @brief 尺度を考慮した位置座標の加減算
	void addOffset(int iX, int iY);

	void shiftAnimation();
	void shiftSkin();

	void setAnimationByIndex(size_t nIndex);
	void setAnimationByName(const char* animationName);
	void restartAnimation(bool loop = true);

	void setSkinByIndex(size_t nIndex);
	void setSkinByName(const char* skinName);
	void setupSkin();

	void premultiplyAlpha(bool premultiplied, size_t nDrawableIndex = 0);
	bool isAlphaPremultiplied(size_t nDrawableIndex = 0) const;

	void forceBlendModeNormal(bool toForce, size_t nDrawableIndex = 0);
	bool isBlendModeNormalForced(size_t nDrawableIndex = 0) const;

	void setPause(bool paused, size_t nDrawableIndex = 0);
	bool isPaused(size_t nDrawableIndex = 0) const;

	void setVisibility(bool visible, size_t nDrawableIndex = 0);
	bool isVisible(size_t nDrawableIndex = 0) const;

	void setDrawOrder(bool toBeReversed);
	bool isDrawOrderReversed() const;

	/// @brief 現在再生している動作の名称を取得
	const char* getCurrentAnimationName();
	/// @brief 現在再生されている動作の時間情報を取得
	/// @param fTrack 再生が始まってからの総経過時間
	/// @param fLast 再生区間に於ける、現在の再生位置
	/// @param fStart 再生区間の開始位置
	/// @param fEnd 再生区間の終了位置
	void getCurrentAnimationTime(float* fTrack, float* fLast, float* fStart, float* fEnd);
	/// @brief 再生位置の変更
	void setCurrentAnimationTime(float animationTime);
	/// @brief 動作の再生時間を取得
	float getAnimationDuration(const char* animationName);

	const std::vector<std::string>& getSlotNames() const;
	const std::vector<std::string>& getSkinNames() const;
	const std::vector<std::string>& getAnimationNames() const;

	/// @brief 描画対象から除外するスロットを登録
	void setSlotsToExclude(const std::vector<std::string>& slotNames);
	/// @brief 現在設定されているスキンを上書きしてスキン合成。上書きなので再読み込みしないと元に戻せなくなります
	void mixSkins(const std::vector<std::string>& skinNames);
	void addAnimationTracks(const std::vector<std::string>& animationNames, bool loop = false);
	/// @brief 或る動作から別の或る動作への遷移時間を設定
	void mixAnimations(const char* fadeOutAnimationName, const char* fadeInAnimationName, float mixTime);
	/// @brief 全ての動作間の遷移時間を初期化。Spine4.0以前ではこの機能は無効
	void clearMixedAnimation();

	/// @brief 複数の装着品候補のあるスロットを探索
	/// @return スロット名を見出し語、装着品名を値とした辞書
	std::unordered_map<std::string, std::vector<std::string>> getSlotNamesWithTheirAttachments();
	/// @brief 現在の装着品を強制差し替え
	bool replaceAttachment(const char* szSlotName, const char* szAttachmentName);

	/// @brief ワールド座標における表示領域の大きさを設定
	void setBaseSize(float fWidth, float fHeight);
	FPoint2 getBaseSize() const;
	void resetBaseSize();

	/// @brief 表示領域の開始座標を設定
	void setOffset(float fX, float fY);
	FPoint2 getOffset() const;

	/// @brief 変形行列算出時の拡縮を変更。
	/// @remark Boneの内部変数を変更するものではありません。
	void setSkeletonScale(float fScale);
	float getSkeletonScale() const;

	/// @brief 更新時の加算時間係数を変更。
	/// @param fTimeScale
	/// @remark animationStateの内部変数を変更するものではありません。
	void setTimeScale(float fTimeScale);
	float getTimeScale() const;
protected:
	enum Constants { kBaseWidth = 1280, kBaseHeight = 720, kMinAtlas = 1024 };

	CTextureLoader m_textureLoader;
	std::vector<std::shared_ptr<spine::Atlas>> m_atlases;
	std::vector<std::shared_ptr<spine::SkeletonData>> m_skeletonData;
	std::vector<std::unique_ptr<CSpineDrawable>> m_drawables;

	FPoint2 m_fBaseSize = FPoint2{ kBaseWidth, kBaseHeight };

	float m_fDefaultScale = 1.f;
	FPoint2 m_fDefaultOffset{};

	float m_fTimeScale = 1.f;
	float m_fSkeletonScale = 1.f;
	FPoint2 m_fOffset{};

	std::vector<std::string> m_animationNames;
	size_t m_nAnimationIndex = 0;

	std::vector<std::string> m_skinNames;
	size_t m_nSkinIndex = 0;

	std::vector<std::string> m_slotNames;

	bool m_isDrawOrderReversed = false;

	void clearDrawables();
	bool addDrawable(spine::SkeletonData* const pSkeletonData);
	bool setupDrawables();

	void workOutDefaultSize();
	virtual void workOutDefaultScale() = 0;
	virtual void workOutDefaultOffset() = 0;

	void updatePosition();

	void clearAnimationTracks();
};

#endif // !SPINE_PLAYER_H_
