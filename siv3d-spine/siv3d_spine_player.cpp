

#include "siv3d_spine_player.h"

CSiv3dSpinePlayer::CSiv3dSpinePlayer()
{

}

CSiv3dSpinePlayer::~CSiv3dSpinePlayer()
{

}

void CSiv3dSpinePlayer::Redraw()
{
	if (!m_drawables.empty())
	{
		float fX = (m_fBaseSize.x * m_fSkeletonScale - m_sceneSize.x) / 2;
		float fY = (m_fBaseSize.y * m_fSkeletonScale - m_sceneSize.y) / 2;
		const s3d::Mat3x2 matrix = s3d::Mat3x2::Scale(m_fSkeletonScale).translated(-fX, -fY);
		const s3d::Transformer2D t(matrix);

		if (!m_isDrawOrderReversed)
		{
			for (const auto& drawable : m_drawables)
			{
				drawable->Draw();
			}
		}
		else
		{
			for (long long i = m_drawables.size() - 1; i >= 0; --i)
			{
				m_drawables[i]->Draw();
			}
		}
	}
}

void CSiv3dSpinePlayer::WorkOutDefaultScale()
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

void CSiv3dSpinePlayer::WorkOutDefaultOffset()
{
	/* s3d::Inf<float> とs3d::IsInfinityでは、外部ライブラリの書き込んだFLT_MAXを検知できない。*/
	float fMinX = FLT_MAX;
	float fMinY = FLT_MAX;

	for (const auto& pDrawable : m_drawables)
	{
		const auto& rect = pDrawable->GetBoundingBox();
		fMinX = s3d::Min(fMinX, rect.x);
		fMinY = s3d::Min(fMinY, rect.y);
	}

	m_fDefaultOffset = { fMinX == FLT_MAX ? 0 : fMinX, fMinY == FLT_MAX ? 0 : fMinY };
}
