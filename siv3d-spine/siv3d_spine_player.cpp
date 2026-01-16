

#include "siv3d_spine_player.h"
#include "siv3d_spine_loader.h"

CSiv3dSpinePlayer::CSiv3dSpinePlayer()
{

}

CSiv3dSpinePlayer::~CSiv3dSpinePlayer()
{

}

bool CSiv3dSpinePlayer::loadSpineFromFile(const s3d::Array<s3d::FilePath>& atlasFilePaths, const s3d::Array<s3d::FilePath>& skeletonFilePaths)
{
	if (atlasFilePaths.size() != skeletonFilePaths.size())return false;
	clearDrawables();

	for (size_t i = 0; i < atlasFilePaths.size(); ++i)
	{
		const auto& atlasFilePath = atlasFilePaths[i];
		const auto& skeletonFilePath = skeletonFilePaths[i];

		std::shared_ptr<spine::Atlas> pAtlas = siv3d_spine_loader::ReadAtlasFromFile(atlasFilePath, &m_textureLoader);
		if (pAtlas == nullptr) continue;

		std::shared_ptr<spine::SkeletonData> pSkeletonData = siv3d_spine_loader::ReadSkeletonFromFile(skeletonFilePath, pAtlas.get());
		if (pSkeletonData == nullptr)return false;

		m_atlases.push_back(std::move(pAtlas));
		m_skeletonData.push_back(std::move(pSkeletonData));
	}

	if (m_skeletonData.empty())return false;

	return setupDrawables();
}

bool CSiv3dSpinePlayer::loadSpineFromMemory(const s3d::Array<s3d::Blob>& atlasFileData, const s3d::Array<s3d::FilePath>& textureDirectories, const s3d::Array<s3d::Blob>& skeletonFileData)
{
	if (atlasFileData.size() != skeletonFileData.size() || atlasFileData.size() != textureDirectories.size())return false;
	clearDrawables();

	for (size_t i = 0; i < atlasFileData.size(); ++i)
	{
		const auto& atlasFileDatum = atlasFileData[i];
		const auto& textureDirectory = textureDirectories[i];
		const auto& skeletonFileDatum = skeletonFileData[i];

		std::shared_ptr<spine::Atlas> pAtlas = siv3d_spine_loader::ReadAtlasFromMemory(atlasFileDatum, textureDirectory, &m_textureLoader);
		if (pAtlas == nullptr) continue;

		std::shared_ptr<spine::SkeletonData> pSkeletonData = siv3d_spine_loader::ReadSkeletonFromMemory(skeletonFileDatum, pAtlas.get());
		if (pSkeletonData == nullptr)return false;

		m_atlases.push_back(std::move(pAtlas));
		m_skeletonData.push_back(std::move(pSkeletonData));
	}

	if (m_skeletonData.empty())return false;

	return setupDrawables();
}

bool CSiv3dSpinePlayer::addSpineFromFile(const s3d::FilePath& atlasFilePath, const s3d::FilePath& skeletonFilePath)
{
	if (m_drawables.empty() || atlasFilePath.empty() || skeletonFilePath.empty())return false;

	std::shared_ptr<spine::Atlas> pAtlas = siv3d_spine_loader::ReadAtlasFromFile(atlasFilePath, &m_textureLoader);
	if (pAtlas == nullptr) false;

	std::shared_ptr<spine::SkeletonData> pSkeletonData = siv3d_spine_loader::ReadSkeletonFromFile(skeletonFilePath, pAtlas.get());
	if (pSkeletonData == nullptr)return false;

	bool bRet = addDrawable(pSkeletonData.get());
	if (!bRet)return false;

	m_atlases.push_back(std::move(pAtlas));
	m_skeletonData.push_back(std::move(pSkeletonData));
	if (m_isDrawOrderReversed)
	{
		std::rotate(m_drawables.rbegin(), m_drawables.rbegin() + 1, m_drawables.rend());
	}

	restartAnimation();
	resetScale();

	return true;
}

void CSiv3dSpinePlayer::redraw()
{
	if (!m_drawables.empty())
	{
		if (!m_isDrawOrderReversed)
		{
			for (const auto& drawable : m_drawables)
			{
				drawable->draw();
			}
		}
		else
		{
			for (long long i = m_drawables.size() - 1; i >= 0; --i)
			{
				m_drawables[i]->draw();
			}
		}
	}
}

s3d::Mat3x2 CSiv3dSpinePlayer::calculateTransformMatrix(s3d::Size renderTargetSize) const
{
	float fX = (m_fBaseSize.x * m_fSkeletonScale - renderTargetSize.x) / 2;
	float fY = (m_fBaseSize.y * m_fSkeletonScale - renderTargetSize.y) / 2;
	return s3d::Mat3x2::Scale(m_fSkeletonScale).translated(-fX, -fY);
}

s3d::Optional<s3d::Vector4D<float>> CSiv3dSpinePlayer::getCurrentBoundingOfSlot(const std::string& slotName) const
{
	for (const auto& drawable : m_drawables)
	{
		const auto& rect = drawable->getBoundingBoxOfSlot(slotName.c_str(), slotName.size());
		if (rect.has_value())return rect;
	}
	return s3d::none;
}

void CSiv3dSpinePlayer::workOutDefaultScale()
{
	m_fDefaultScale = 1.f;

	int iSkeletonWidth = static_cast<int>(m_fBaseSize.x);
	int iSkeletonHeight = static_cast<int>(m_fBaseSize.y);

	auto monitor = s3d::System::GetCurrentMonitor();
	int iDisplayWidth = monitor.displayRect.w;
	int iDisplayHeight = monitor.displayRect.h;
	if (iSkeletonWidth > iDisplayWidth || iSkeletonHeight > iDisplayHeight)
	{
		float fScaleX = static_cast<float>(iDisplayWidth) / iSkeletonWidth;
		float fScaleY = static_cast<float>(iDisplayHeight) / iSkeletonHeight;

		if (fScaleX > fScaleY)
		{
			m_fDefaultScale = fScaleY;
		}
		else
		{
			m_fDefaultScale = fScaleX;
		}
	}
}

void CSiv3dSpinePlayer::workOutDefaultOffset()
{
	float fMinX = s3d::Largest<float>;
	float fMinY = s3d::Largest<float>;

	for (const auto& pDrawable : m_drawables)
	{
		const auto& rect = pDrawable->getBoundingBox();
		fMinX = s3d::Min(fMinX, rect.x);
		fMinY = s3d::Min(fMinY, rect.y);
	}

	m_fDefaultOffset = { fMinX == s3d::Largest<float> ? 0 : fMinX, fMinY == s3d::Largest<float> ? 0 : fMinY };
}
