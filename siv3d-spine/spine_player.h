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

	bool loadSpineFromFile(const std::vector<std::string>& atlasPaths, const std::vector<std::string>& skelPaths, bool isBinarySkel);
	bool loadSpineFromMemory(const std::vector<std::string>& atlasData, const std::vector<std::string>& atlasPaths, const std::vector<std::string>& skelData, bool isBinarySkel);

	bool addSpineFromFile(const char* szAtlasPath, const char* szSkelPath, bool isBinarySkel);

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

	bool premultiplyAlpha(bool toBePremultiplied, size_t nDrawableIndex = 0);
	bool isAlphaPremultiplied(size_t nDrawableIndex = 0) const;

	bool forceBlendModeNormal(bool toForce, size_t nDrawableIndex = 0);
	bool isBlendModeNormalForced(size_t nDrawableIndex = 0) const;

	void setDrawOrder(bool toBeReversed);
	bool isDrawOrderReversed() const;

	const char* getCurrentAnimationName();
	/// @brief 現在再生されている動作の時間情報を取得
	/// @param fTrack 再生が始まってからの総経過時間
	/// @param fLast 再生区間に於ける、現在の再生位置
	/// @param fStart 再生区間の開始位置
	/// @param fEnd 再生区間の終了位置
	void getCurrentAnimationTime(float* fTrack, float* fLast, float* fStart, float* fEnd);

	const std::vector<std::string>& getSlotNames() const;
	const std::vector<std::string>& getSkinNames() const;
	const std::vector<std::string>& getAnimationNames() const;

	void setSlotsToExclude(const std::vector<std::string>& slotNames);
	void mixSkins(const std::vector<std::string>& skinNames);
	void mixAnimations(const std::vector<std::string>& animationNames, bool loop = false);

	/// @brief 複数の装着品候補のあるスロットを探索。
	/// @return スロット名を見出し語、装着品名を値とした辞書。
	std::unordered_map<std::string, std::vector<std::string>> getSlotNamesWithTheirAttachments();
	/// @brief 現在の装着品を強制差し替え。
	bool replaceAttachment(const char* szSlotName, const char* szAttachmentName);

	FPoint2 getBaseSize() const;
	void setBaseSize(float fWidth, float fHeight);
	void resetBaseSize();

	FPoint2 getOffset() const;
	void setOffset(float fWidth, float fHeight);

	float getSkeletonScale() const;
	void setSkeletonScale(float fScale);

	float getTimeScale() const;
	void setTimeScale(float fTimeScale);
protected:
	enum Constants { kBaseWidth = 1280, kBaseHeight = 720, kMinAtlas = 1024 };

	CTextureLoader m_textureLoader;
	std::vector<std::unique_ptr<spine::Atlas>> m_atlases;
	std::vector<std::shared_ptr<spine::SkeletonData>> m_skeletonData;
	std::vector<std::shared_ptr<CSpineDrawable>> m_drawables;

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
