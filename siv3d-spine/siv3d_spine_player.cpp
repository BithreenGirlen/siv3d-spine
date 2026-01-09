

#include "siv3d_spine_player.h"

CSiv3dSpinePlayer::CSiv3dSpinePlayer()
{

}

CSiv3dSpinePlayer::~CSiv3dSpinePlayer()
{

}

void CSiv3dSpinePlayer::redraw()
{
	if (!m_drawables.empty())
	{
		const s3d::Transformer2D t(calculateTransformMatrix());

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

s3d::Mat3x2 CSiv3dSpinePlayer::calculateTransformMatrix() const
{
	float fX = (m_fBaseSize.x * m_fSkeletonScale - m_sceneSize.x) / 2;
	float fY = (m_fBaseSize.y * m_fSkeletonScale - m_sceneSize.y) / 2;
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
