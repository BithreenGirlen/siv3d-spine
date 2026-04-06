#ifndef SIV3D_IMGUI_DIALOGUE_H_
#define SIV3D_IMGUI_DIALOGUE_H_

#include "../../siv3d-spine/siv3d_spine_player.h"

namespace siv3d_imgui_dialogue
{
	struct SpineToolDatum
	{
		CSiv3dSpinePlayer& siv3dSpinePlayer;
		std::unique_ptr<s3d::RenderTexture>& spinePlayerTexture;

		float& spineCanvasScale;
		s3d::Point& spineRenderPosition;
		bool& isWireFrameMode;

		s3d::int32& imageFps;
		s3d::int32& videoFps;

		bool isWindowToBeResized = false;
	};

	void ShowSpineTool(SpineToolDatum& spineToolDatum);
	void ShowHelp();
}
#endif // !SIV3D_IMGUI_DIALOGUE_H_
