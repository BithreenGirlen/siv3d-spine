#ifndef SIV3D_MAIN_WINDOW_H_
#define SIV3D_MAIN_WINDOW_H_

#include "siv3d_window_menu.h"
#include "../../siv3d-spine/siv3d_spine_player.h"
#include "siv3d_recorder.h"

class CSiv3dMainWindow
{
public:
	CSiv3dMainWindow(const char32_t *windowName = nullptr);
	~CSiv3dMainWindow();

	void Display();

private:
	CSiv3dWindowMenu m_siv3dWindowMenu;

	CSiv3dSpinePlayer m_siv3dSpinePlayer;
	/// @brief Spine描画先
	std::unique_ptr<s3d::RenderTexture> m_pSpinePlayerTexture;

	bool m_isSpineParameterHidden = false;
	bool m_isHelpDialogueShown = false;

	CSiv3dRecorder m_siv3dRecorder;
	s3d::int32 m_imageFps = 30;
	s3d::int32 m_videoFps = 60;

	void InitialiseMenuBar();

	void MenuOnOpenFile();

	void MenuOnSnapImage();
	void MenuOnExportAsGif();
	void MenuOnExportAsVideo();

	void MenuOnHideSpineParameter();
	void MenuOnShowHelp();

	void HandleMouseEvent();
	void HandleKeyboardEvent();

	void ResizeWindow();
	void CheckRenderTextureSize();
	void ToggleWindowBorderStyle();

	void SpinePostRendering();

	s3d::FilePath BuildExportFilePath();

	void SetEmbeddedFontForImgui() const;
	void ImGuiSpineParameterDialogue();
	void ImGuiHelpDialogue() const;
};

#endif // !SIV3D_MAIN_WINDOW_H_
