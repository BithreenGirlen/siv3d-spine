

#include "siv3d_spine.h"
#include "siv3d_spine_blendmode.h"

#ifdef SPINE_4_2_OR_LATER
	#ifndef SPINE_4_1_OR_LATER
	#define SPINE_4_1_OR_LATER
	#endif
#endif

namespace spine
{
	spine::SpineExtension* getDefaultExtension()
	{
		return new DefaultSpineExtension();
	}
}

CS3dSpineDrawable::CS3dSpineDrawable(spine::SkeletonData* pSkeletonData, spine::AnimationStateData* pAnimationStateData)
{
	spine::Bone::setYDown(true);

	skeleton = new spine::Skeleton(pSkeletonData);

	if (pAnimationStateData == nullptr)
	{
		pAnimationStateData = new spine::AnimationStateData(pSkeletonData);
		m_hasOwnAnimationStateData = true;
	}
	animationState = new spine::AnimationState(pAnimationStateData);

	m_quadIndices.add(0);
	m_quadIndices.add(1);
	m_quadIndices.add(2);
	m_quadIndices.add(2);
	m_quadIndices.add(3);
	m_quadIndices.add(0);
}

CS3dSpineDrawable::~CS3dSpineDrawable()
{
	if (animationState != nullptr)
	{
		if (m_hasOwnAnimationStateData)
		{
			delete animationState->getData();
		}

		delete animationState;
	}
	if (skeleton != nullptr)
	{
		delete skeleton;
	}
}

void CS3dSpineDrawable::Update(float fDelta)
{
	if (skeleton != nullptr && animationState != nullptr)
	{
#ifndef SPINE_4_1_OR_LATER
		skeleton->update(fDelta);
#endif
		animationState->update(fDelta);
		animationState->apply(*skeleton);
#ifdef SPINE_4_2_OR_LATER
		skeleton->update(fDelta);
		skeleton->updateWorldTransform(spine::Physics::Physics_Update);
#else
		skeleton->updateWorldTransform();
#endif
	}
}

void CS3dSpineDrawable::Draw()
{
	if (skeleton == nullptr || animationState == nullptr)return;

	if (skeleton->getColor().a == 0)return;

	for (size_t i = 0; i < skeleton->getSlots().size(); ++i)
	{
		spine::Slot& slot = *skeleton->getDrawOrder()[i];
		spine::Attachment* pAttachment = slot.getAttachment();

		if (pAttachment == nullptr || slot.getColor().a == 0 || !slot.getBone().isActive())
		{
			m_skeletonClipping.clipEnd(slot);
			continue;
		}

		if (IsSlotToBeLeftOut(slot.getData().getName()))
		{
			m_skeletonClipping.clipEnd(slot);
			continue;
		}

		spine::Vector<float>* pVertices = &m_worldVertices;
		spine::Vector<float>* pAttachmentUvs = nullptr;
		spine::Vector<unsigned short>* pIndices = nullptr;

		spine::Color* pAttachmentColor = nullptr;

		s3d::Texture* pTexture = nullptr;

		if (pAttachment->getRTTI().isExactly(spine::RegionAttachment::rtti))
		{
			spine::RegionAttachment* pRegionAttachment = static_cast<spine::RegionAttachment*>(pAttachment);
			pAttachmentColor = &pRegionAttachment->getColor();

			if (pAttachmentColor->a == 0)
			{
				m_skeletonClipping.clipEnd(slot);
				continue;
			}

			m_worldVertices.setSize(8, 0);
#ifdef SPINE_4_1_OR_LATER
			pRegionAttachment->computeWorldVertices(slot, m_worldVertices, 0, 2);
#else
			pRegionAttachment->computeWorldVertices(slot.getBone(), m_worldVertices, 0, 2);
#endif
			pAttachmentUvs = &pRegionAttachment->getUVs();
			pIndices = &m_quadIndices;

#ifdef SPINE_4_1_OR_LATER
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pRegionAttachment->getRegion());

			isAlphaPremultiplied = pAtlasRegion->page->pma;
			pTexture = reinterpret_cast<s3d::Texture*>(pAtlasRegion->rendererObject);
#else
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pRegionAttachment->getRendererObject());
#ifdef SPINE_4_0
			isAlphaPremultiplied = pAtlasRegion->page->pma;
#endif
			pTexture = reinterpret_cast<s3d::Texture*>(pAtlasRegion->page->getRendererObject());
#endif
		}
		else if (pAttachment->getRTTI().isExactly(spine::MeshAttachment::rtti))
		{
			spine::MeshAttachment* pMeshAttachment = static_cast<spine::MeshAttachment*>(pAttachment);
			pAttachmentColor = &pMeshAttachment->getColor();

			if (pAttachmentColor->a == 0)
			{
				m_skeletonClipping.clipEnd(slot);
				continue;
			}
			m_worldVertices.setSize(pMeshAttachment->getWorldVerticesLength(), 0);
			pMeshAttachment->computeWorldVertices(slot, 0, pMeshAttachment->getWorldVerticesLength(), m_worldVertices, 0, 2);
			pAttachmentUvs = &pMeshAttachment->getUVs();
			pIndices = &pMeshAttachment->getTriangles();

#ifdef SPINE_4_1_OR_LATER
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pMeshAttachment->getRegion());

			isAlphaPremultiplied = pAtlasRegion->page->pma;
			pTexture = reinterpret_cast<s3d::Texture*>(pAtlasRegion->rendererObject);
#else
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pMeshAttachment->getRendererObject());
#ifdef SPINE_4_0
			isAlphaPremultiplied = pAtlasRegion->page->pma;
#endif
			pTexture = reinterpret_cast<s3d::Texture*>(pAtlasRegion->page->getRendererObject());
#endif
		}
		else if (pAttachment->getRTTI().isExactly(spine::ClippingAttachment::rtti))
		{
			spine::ClippingAttachment* pClippingAttachment = static_cast<spine::ClippingAttachment*>(slot.getAttachment());
			m_skeletonClipping.clipStart(slot, pClippingAttachment);
			continue;
		}
		else
		{
			m_skeletonClipping.clipEnd(slot);
			continue;
		}

		if (m_skeletonClipping.isClipping())
		{
			m_skeletonClipping.clipTriangles(m_worldVertices, *pIndices, *pAttachmentUvs, 2);
			if (m_skeletonClipping.getClippedTriangles().size() == 0)
			{
				m_skeletonClipping.clipEnd(slot);
				continue;
			}
			pVertices = &m_skeletonClipping.getClippedVertices();
			pAttachmentUvs = &m_skeletonClipping.getClippedUVs();
			pIndices = &m_skeletonClipping.getClippedTriangles();
		}

		const spine::Color& skeletonColor = skeleton->getColor();
		const spine::Color& slotColor = slot.getColor();
		const spine::Color tint
		(
			skeletonColor.r * slotColor.r * pAttachmentColor->r,
			skeletonColor.g * slotColor.g * pAttachmentColor->g,
			skeletonColor.b * slotColor.b * pAttachmentColor->b,
			skeletonColor.a * slotColor.a * pAttachmentColor->a
		);

		m_buffer2d.vertices.resize(pVertices->size() / 2);
		for (int ii = 0, k = 0; ii < pVertices->size(); ii += 2, ++k)
		{
			auto& s3dVertex = m_buffer2d.vertices[k];

			s3dVertex.pos.x = (*pVertices)[ii];
			s3dVertex.pos.y = (*pVertices)[ii + 1LL];

			s3dVertex.color.x = tint.r * (isAlphaPremultiplied ? tint.a : 1.f);
			s3dVertex.color.y = tint.g * (isAlphaPremultiplied ? tint.a : 1.f);
			s3dVertex.color.z = tint.b * (isAlphaPremultiplied ? tint.a : 1.f);
			s3dVertex.color.w = tint.a;

			s3dVertex.tex.x = (*pAttachmentUvs)[ii];
			s3dVertex.tex.y = (*pAttachmentUvs)[ii + 1LL];
		}

		m_buffer2d.indices.resize(pIndices->size() / 3);
		if constexpr (sizeof(s3d::TriangleIndex::value_type) == sizeof(unsigned short))
		{
			memcpy(m_buffer2d.indices.data(), pIndices->buffer(), pIndices->size() * sizeof(unsigned short));
		}
		else
		{
			for (int ii = 0, k = 0; ii < pIndices->size(); ii += 3, ++k)
			{
				auto& triangleIndex = m_buffer2d.indices[k];
				triangleIndex.i0 = static_cast<s3d::TriangleIndex::value_type>((*pIndices)[ii]);
				triangleIndex.i1 = static_cast<s3d::TriangleIndex::value_type>((*pIndices)[ii + 1LL]);
				triangleIndex.i2 = static_cast<s3d::TriangleIndex::value_type>((*pIndices)[ii + 2LL]);
			}
		}

		s3d::BlendState s3dBlendState;
		spine::BlendMode spineBlendMode = isToForceBlendModeNormal ? spine::BlendMode::BlendMode_Normal : slot.getData().getBlendMode();
		switch (spineBlendMode)
		{
		case spine::BlendMode_Additive:
			s3dBlendState = isAlphaPremultiplied ? Siv3dSpineBlendMode::AddPma : Siv3dSpineBlendMode::Add;
			break;
		case spine::BlendMode_Multiply:
			s3dBlendState = Siv3dSpineBlendMode::Multiply;
			break;
		case spine::BlendMode_Screen:
			s3dBlendState = Siv3dSpineBlendMode::Screen;
			break;
		default:
			s3dBlendState = isAlphaPremultiplied ? Siv3dSpineBlendMode::NormalPma : Siv3dSpineBlendMode::Normal;
			break;
		}

		/*
		 * Here sampler state is fixed to clampLinear.
		 * More accurate way is to refer to AtlasPage::uWrap and AtlasPage::magFilter
		 */
		s3d::ScopedRenderStates2D s3dScopedRenderState2D(s3dBlendState, s3d::SamplerState::ClampLinear);
		pTexture != nullptr ? m_buffer2d.draw(*pTexture) : m_buffer2d.draw();

		m_skeletonClipping.clipEnd(slot);
	}
	m_skeletonClipping.clipEnd();
}

void CS3dSpineDrawable::SetSlotsToLeaveOut(spine::Vector<spine::String>& slotNames)
{
	m_slotsToLeaveOut.clearAndAddAll(slotNames);
}

s3d::Vector4D<float> CS3dSpineDrawable::GetBoundingBox() const
{
	s3d::Vector4D<float> boundingBox{};

	if (skeleton != nullptr)
	{
		spine::Vector<float> tempVertices;
		skeleton->getBounds(boundingBox.x, boundingBox.y, boundingBox.z, boundingBox.w, tempVertices);
	}

	return boundingBox;
}

bool CS3dSpineDrawable::IsSlotToBeLeftOut(const spine::String& slotName)
{
	return m_slotsToLeaveOut.contains(slotName);
}

void CS3dTextureLoader::load(spine::AtlasPage& atlasPage, const spine::String& path)
{
	s3d::FilePath filePath = s3d::Unicode::FromUTF8(path.buffer());
	if (filePath.starts_with(U"//"))
	{
		filePath.replace(U"/", U"\\");
	}
	s3d::Texture* pTexture = new (std::nothrow) s3d::Texture(filePath);

	/* Do not overwrite the size of atlas page with that of texture because it will collapse uvs. */
	//if (atlasPage.width == 0 || atlasPage.height == 0)
	//{
	//	atlasPage.width = pTexture->width();
	//	atlasPage.height = pTexture->height();
	//}
#ifdef SPINE_4_1_OR_LATER
	atlasPage.texture = pTexture;
#else
	atlasPage.setRendererObject(pTexture);
#endif
}

void CS3dTextureLoader::unload(void* texture)
{
	delete static_cast<s3d::Texture*>(texture);
}
