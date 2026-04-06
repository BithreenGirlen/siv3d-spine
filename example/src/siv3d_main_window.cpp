

#include "siv3d_main_window.h"

#include "imgui/siv3d_imgui.h"


CSiv3dMainWindow::CSiv3dMainWindow(const char32_t* windowName)
{
	if (windowName != nullptr)
	{
		s3d::Window::SetTitle(windowName);
	}

	/*
	* 各画面上限
	* Scene (Virtual mode): なし
	* Scene (Actual mode): デスクトップ解像度 + 枠幅
	* Frame: デスクトップ解像度 + 枠幅
	* Virtual: デスクトップ解像度 / DPI
	*/
	s3d::Scene::SetResizeMode(s3d::ResizeMode::Actual);
	s3d::Window::SetStyle(s3d::WindowStyle::Sizable);

	initialiseMenuBar();
}

CSiv3dMainWindow::~CSiv3dMainWindow()
{

}

void CSiv3dMainWindow::display()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsLight();
	ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.875f;

	ImGui_ImplSiv3d_Init();

	setEmbeddedFontForImgui();

	while (s3d::System::Update())
	{
		ImGui_ImplSiv3d_NewFrame();
		ImGui::NewFrame();

		m_siv3dWindowMenu.update();

		handleMouseEvent();
		handleKeyboardEvent();

		float fDelta = static_cast<float>(s3d::Scene::DeltaTime());
		if (m_siv3dRecorder.isUnderRecording())
		{
			fDelta = m_siv3dRecorder.hasFrames() ? 1.f / m_siv3dRecorder.getFps() : 0.f;
		}
		m_siv3dSpinePlayer.update(fDelta);

		m_spineRenderPosition.y = m_siv3dWindowMenu.isVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;
		if (m_pSpinePlayerTexture.get() != nullptr)
		{
			m_pSpinePlayerTexture->clear(s3d::ColorF(0.f, 0.f));
			{
				const s3d::ScopedRenderTarget2D spinePlayerRenderTarget(*m_pSpinePlayerTexture.get());
				const s3d::Transformer2D t(m_siv3dSpinePlayer.calculateTransformMatrix(m_pSpinePlayerTexture->size()));
				const s3d::ScopedRenderStates2D s3dScopedRenderState2D(m_isWireframeMode ? s3d::RasterizerState::WireframeCullNone : s3d::RasterizerState::Default2D);
				m_siv3dSpinePlayer.redraw();
			}

			m_pSpinePlayerTexture->draw(m_spineRenderPosition);

			spinePostRendering();
		}

		imGuiSpineToolDialogue();
		imGuiHelpDialogue();

		ImGui::Render();
		ImGui_ImplSiv3d_RenderDrawData();

		/* 操作欄を最前面に。 */
		m_siv3dWindowMenu.draw();
	}

	ImGui_ImplSiv3d_Shutdown();
}

void CSiv3dMainWindow::initialiseMenuBar()
{
	if (!m_siv3dWindowMenu.hasBeenInitialised())
	{
		const s3d::Array<std::pair<s3d::String, s3d::Array<s3d::String>>> menuItems
		{
			{ U"File", { U"\U000F102FOpen file"} },
			{ U"Export", { U"\U000F0193Snap as Webp", U"\U000F05D8Export as GIF", U"\U000F0BDCExport as video"} },
			{ U"Window", { U"\U000F06D1Hide Spine tool", U"\U000F0625Show help", U"\U000F18F4Fit base size to current frame", U"Reset base size", U"Disable auto resizing"}}
		};

		const s3d::Array<s3d::Array<CSiv3dWindowMenu::ItemProprty>> menuItemProperties
		{
			{
				{ std::bind(&CSiv3dMainWindow::menuOnOpenFile, this), CSiv3dWindowMenu::Restrictive::No }
			},
			{
				{ std::bind(&CSiv3dMainWindow::menuOnSnapImage, this), CSiv3dWindowMenu::Restrictive::Yes },
				{ std::bind(&CSiv3dMainWindow::menuOnExportAsGif, this), CSiv3dWindowMenu::Restrictive::Yes },
				{ std::bind(&CSiv3dMainWindow::menuOnExportAsVideo, this), CSiv3dWindowMenu::Restrictive::Yes }
			},
			{
				{ std::bind(&CSiv3dMainWindow::menuOnHideSpineTool, this), CSiv3dWindowMenu::Restrictive::Yes },
				{ std::bind(&CSiv3dMainWindow::menuOnShowHelp, this), CSiv3dWindowMenu::Restrictive::No },
				{ [this] /* 手動変更された現在の表示範囲に合わせる */
					{
						if (m_pSpinePlayerTexture.get() == nullptr) return;

						const s3d::Size targetSize = m_pSpinePlayerTexture->size();
						const float fSkeletonScale = m_siv3dSpinePlayer.getSkeletonScale();
						s3d::Vector2D<float> baseSize = targetSize / fSkeletonScale;

						m_siv3dSpinePlayer.setBaseSize(baseSize.x, baseSize.y);
						m_spineCanvasScale = m_siv3dSpinePlayer.getSkeletonScale();
						resizeWindow();
					}, CSiv3dWindowMenu::Restrictive::Yes
				},
				{ [this] /* 既定の表示範囲に戻す */
					{
						m_siv3dSpinePlayer.resetBaseSize();
						m_spineCanvasScale = m_siv3dSpinePlayer.getSkeletonScale();
						resizeWindow();
					}, CSiv3dWindowMenu::Restrictive::Yes
				},
				{ [this] /* Spineの拡縮変更時にウィンドウの大きさも一緒に変更するか否か */
					{
						m_isAutoWindowResizingDisabled ^= true;
						m_siv3dWindowMenu.setLastItemChecked(m_isAutoWindowResizingDisabled);
					}, CSiv3dWindowMenu::Restrictive::Yes
				}
			}
		};

		m_siv3dWindowMenu.initialise(menuItems, menuItemProperties);
	}
}

void CSiv3dMainWindow::menuOnOpenFile()
{
	const s3d::Array<s3d::String> atlasCandidates = { U"atlas" , U"atlas.txt" };
	s3d::Optional<s3d::FilePath> selectedAtlas = s3d::Dialog::OpenFile({ { U"Atlas file", { atlasCandidates } }, { U"All files", { U"*" } } }, U"", U"Select atlas");
	if (!selectedAtlas.has_value())return;

	const s3d::Array<s3d::String> skelCandidates = { U"skel", U"bin", U"bytes", U"json" };
	s3d::Optional<s3d::FilePath> selectedSkeleton = s3d::Dialog::OpenFile({ { U"Skeleton file", skelCandidates }, { U"All files", { U"*" } } }, U"", U"Select skeleton");
	if (!selectedSkeleton.has_value())return;

#if SIV3D_PLATFORM(WINDOWS)
	const auto UnnormalisePath = [](s3d::FilePath& filePath)
		{
			if (filePath.starts_with(U"//"))
			{
				filePath.replace(U'/', U'\\');
			}
		};
	UnnormalisePath(selectedAtlas.value());
	UnnormalisePath(selectedSkeleton.value());
#endif

	s3d::Array<s3d::FilePath> atlasFilePaths;
	s3d::Array<s3d::FilePath> skeletonFilePaths;

	atlasFilePaths.push_back(std::move(selectedAtlas.value()));
	skeletonFilePaths.push_back(std::move(selectedSkeleton.value()));

	m_siv3dSpinePlayer.loadSpineFromFile(atlasFilePaths, skeletonFilePaths);

	m_siv3dWindowMenu.updateRestrictiveItemState(m_siv3dSpinePlayer.hasSpineBeenLoaded());
	if (m_siv3dSpinePlayer.hasSpineBeenLoaded())
	{
		s3d::Window::SetTitle(s3d::FileSystem::BaseName(atlasFilePaths[0]));
		m_spineCanvasScale = m_siv3dSpinePlayer.getSkeletonScale();
		resizeWindow();
	}
}

void CSiv3dMainWindow::menuOnSnapImage()
{
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		s3d::Image image;
		m_pSpinePlayerTexture->readAsImage(image);

		s3d::FilePath filePath = buildExportFilePath();
		{
			s3d::Vector4D<float> animationWatch{};
			m_siv3dSpinePlayer.getCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);

			using namespace s3d;
			const auto& formatted = U"_{:.2f}"_fmt(animationWatch.y);
			filePath += formatted;
		}

		filePath += U".webp";
		image.saveWebP(filePath);
	}
}

void CSiv3dMainWindow::menuOnExportAsGif()
{
	startSpineRecording(CSiv3dRecorder::EOutputType::Gif);
}

void CSiv3dMainWindow::menuOnExportAsVideo()
{
	startSpineRecording(CSiv3dRecorder::EOutputType::Video);
}

void CSiv3dMainWindow::menuOnHideSpineTool()
{
	m_isSpineToolHidden ^= true;
	m_siv3dWindowMenu.setLastItemChecked(m_isSpineToolHidden);
}

void CSiv3dMainWindow::menuOnShowHelp()
{
	m_isHelpDialogueShown ^= true;
	m_siv3dWindowMenu.setLastItemChecked(m_isHelpDialogueShown);
}
/* マウス入力処理 */
void CSiv3dMainWindow::handleMouseEvent()
{
	if (m_siv3dRecorder.isUnderRecording())return;

	const auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse)return;

	if (s3d::MouseL.pressed())
	{
		if (s3d::KeyLControl.pressed() && s3d::Window::GetStyle() == s3d::WindowStyle::Frameless) /* 枠無しウィンドウ移動 */
		{
			s3d::Point mouseDelta = s3d::Cursor::ScreenDelta();
			s3d::Rect windowRect = s3d::Window::GetState().bounds;

			s3d::Point windowPosToBe = s3d::Point{ windowRect.x + mouseDelta.x, windowRect.y + mouseDelta.y };
			s3d::Window::SetPos(windowPosToBe);
		}
		else
		{
			if (!s3d::Window::GetState().sizeMove) /* 視点移動 */
			{
				s3d::Point mouseDelta = s3d::Cursor::Delta();
				m_siv3dSpinePlayer.addOffset(-mouseDelta.x, -mouseDelta.y);
			}
		}
	}
	else if (s3d::MouseL.up())
	{
		if (s3d::MouseR.pressed())
		{
			m_siv3dSpinePlayer.shiftAnimation();
		}
		else
		{
			checkRenderTextureSize();
		}
	}
	else if (s3d::MouseM.up())
	{
		if (s3d::MouseR.pressed()) /* 枠有無切り替え */
		{
			toggleWindowBorderStyle();
		}
		else /* 視点・速度・尺度初期化 */
		{
			m_siv3dSpinePlayer.resetScale();
			m_spineCanvasScale = 1.f;
			resizeWindow();
		}
	}

	if (s3d::Mouse::Wheel())
	{
		const float scrollSign = (s3d::Mouse::Wheel() > 0 ? 1.f : -1.f);
		if (s3d::MouseL.pressed()) /* 動作速度変更 */
		{
			static constexpr float TimeScaleDelta = 0.05f;

			float timeScale = m_siv3dSpinePlayer.getTimeScale() + TimeScaleDelta * scrollSign;
			/* 逆再生にはしない。 */
			timeScale = s3d::Max(timeScale, 0.f);
			m_siv3dSpinePlayer.setTimeScale(timeScale);
		}
		else if (!s3d::MouseR.pressed()) /* 拡縮 */
		{
			static constexpr float ScaleDelta = 0.025f;
			static constexpr float MinScale = 0.15f;

			float skeletonScale = m_siv3dSpinePlayer.getSkeletonScale() + ScaleDelta * scrollSign;
			skeletonScale = s3d::Max(MinScale, skeletonScale);
			m_siv3dSpinePlayer.setSkeletonScale(skeletonScale);

			if (!s3d::KeyLControl.pressed() && !m_isAutoWindowResizingDisabled) /* ウィンドウ寸法変更 */
			{
				m_spineCanvasScale += ScaleDelta * scrollSign;
				m_spineCanvasScale = s3d::Max(MinScale, m_spineCanvasScale);

				resizeWindow();
			}
		}
	}
}
/* キーボード入力処理 */
void CSiv3dMainWindow::handleKeyboardEvent()
{
	if (m_siv3dRecorder.isUnderRecording())return;

	const auto& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)return;

	if (s3d::KeyM.up())
	{
		/* メニュー消去・表示。 */
		m_siv3dWindowMenu.setVisibility(!m_siv3dWindowMenu.isVisible());
		resizeWindow();
	}
	else if (s3d::KeyA.up())
	{
		for (size_t i = 0; i < m_siv3dSpinePlayer.getNumberOfSpines(); ++i)
		{
			m_siv3dSpinePlayer.premultiplyAlpha(!m_siv3dSpinePlayer.isAlphaPremultiplied(i), i);
		}
	}
	else if (s3d::KeyB.up())
	{
		for (size_t i = 0; i < m_siv3dSpinePlayer.getNumberOfSpines(); ++i)
		{
			m_siv3dSpinePlayer.forceBlendModeNormal(!m_siv3dSpinePlayer.isBlendModeNormalForced(i), i);
		}
	}
	else if (s3d::KeyS.up())
	{
		m_siv3dSpinePlayer.shiftSkin();
	}
}
/* 窓寸法変更 */
void CSiv3dMainWindow::resizeWindow()
{
	if (!m_siv3dSpinePlayer.hasSpineBeenLoaded() || m_siv3dRecorder.isUnderRecording())return;

	s3d::Vector2D<float> fCanvasSize = m_siv3dSpinePlayer.getBaseSize();

	s3d::int32 iClientWidth = static_cast<s3d::int32>(fCanvasSize.x * m_spineCanvasScale);
	s3d::int32 iClientHeight = static_cast<s3d::int32>(fCanvasSize.y * m_spineCanvasScale);

	/* メニュー表示時はクライアント領域を高さ分大きく取り、且つ、他の描画対象物の描画開始位置を高さ分下げる。*/
	s3d::int32 menuBarHeight = m_siv3dWindowMenu.isVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;

	/* 書き出しファイルの寸法を解像度内に抑えるため上限を設ける。 */
	const auto monitorInfo = s3d::System::GetCurrentMonitor();
	iClientWidth = s3d::Min(iClientWidth, monitorInfo.displayRect.w);
	iClientHeight = s3d::Min(iClientHeight, monitorInfo.displayRect.h);

	s3d::Window::ResizeActual(iClientWidth, iClientHeight + menuBarHeight, s3d::YesNo<s3d::Centering_tag>::No);

	checkRenderTextureSize();
}
/* Spine描画先紋理要再作成確認 */
void CSiv3dMainWindow::checkRenderTextureSize()
{
	if (!m_siv3dSpinePlayer.hasSpineBeenLoaded()) return;

	s3d::int32 menuBarHeight = m_siv3dWindowMenu.isVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;
	const auto ToBeRecreated = [this, &menuBarHeight]()
		-> bool
		{
			if (m_pSpinePlayerTexture.get() == nullptr)
			{
				return true;
			}
			else
			{
				const auto& windowSize = s3d::Window::GetState().frameBufferSize;
				return windowSize.x != m_pSpinePlayerTexture->size().x ||
					(windowSize.y - menuBarHeight) != m_pSpinePlayerTexture->size().y;
			}
		};

	if (ToBeRecreated())
	{
		const auto& windowSize = s3d::Window::GetState().frameBufferSize;
		const auto& textureSize = s3d::Size{ windowSize.x, windowSize.y - menuBarHeight };

		m_pSpinePlayerTexture = std::make_unique<s3d::RenderTexture>(textureSize);
	}
}
/* 窓枠表示・消去 */
void CSiv3dMainWindow::toggleWindowBorderStyle()
{
	if (s3d::Window::GetStyle() != s3d::WindowStyle::Frameless)
	{
		s3d::Window::SetPos({});
		s3d::Window::SetStyle(s3d::WindowStyle::Frameless);
	}
	else
	{
		s3d::Rect windowRect = s3d::Window::GetState().bounds;
		s3d::Window::SetPos({ windowRect.x, s3d::Max(0, windowRect.y) });
		s3d::Window::SetStyle(s3d::WindowStyle::Sizable);
	}
}
/* Spine描画後処理 */
void CSiv3dMainWindow::spinePostRendering()
{
	if (m_siv3dRecorder.isUnderRecording())
	{
		s3d::Vector4D<float> animationWatch{};
		m_siv3dSpinePlayer.getCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);
		/* 一周し終わったら書き出し。 */
		if (s3d::GreaterThan(animationWatch.x, animationWatch.w))
		{
			s3d::FilePath filePath = buildExportFilePath();
			m_siv3dRecorder.end(filePath);
		}
		else
		{
			s3d::Graphics2D::Flush();
			m_siv3dRecorder.commitFrame(*m_pSpinePlayerTexture.get());
		}
	}
}

void CSiv3dMainWindow::startSpineRecording(CSiv3dRecorder::EOutputType outputType)
{
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		m_siv3dSpinePlayer.setVisibility(true);
		m_siv3dSpinePlayer.setPause(false);
		m_siv3dSpinePlayer.setTimeScale(1.f);
		m_siv3dSpinePlayer.restartAnimation();

		const s3d::Size frameSize(m_pSpinePlayerTexture->width(), m_pSpinePlayerTexture->height());
		const s3d::int32 fps = outputType == CSiv3dRecorder::EOutputType::Video ? m_videoFps : m_imageFps;
		m_siv3dRecorder.start(frameSize, outputType, fps);
	}
}

s3d::FilePath CSiv3dMainWindow::buildExportFilePath()
{
	const s3d::FilePath& saveFolderPath = s3d::FileSystem::PathAppend(s3d::FileSystem::ParentPath(s3d::FileSystem::ModulePath()), s3d::Window::GetTitle());

	const char* pzAnimationName = m_siv3dSpinePlayer.getCurrentAnimationName();
	const s3d::String& fileName = pzAnimationName == nullptr ? U"" : s3d::Unicode::FromUTF8(pzAnimationName);

	return s3d::FileSystem::PathAppend(saveFolderPath, fileName);
}
/* ImGui用書体設定 */
void CSiv3dMainWindow::setEmbeddedFontForImgui() const
{
	ImGuiIO& io = ImGui::GetIO();
	const auto& fontAtlas = io.Fonts;
	const ImWchar* glyph = fontAtlas->GetGlyphRangesChineseFull();
#if SIV3D_PLATFORM(WINDOWS) || SIV3D_PLATFORM(MACOS)
	s3d::Blob fontBlob = s3d::Compression::DecompressFile(s3d::Resource(U"engine/font/noto-cjk/NotoSansCJK-Regular.ttc.zstdcmp"));
	if (!fontBlob.isEmpty())
	{
		/* AddFontFromMemoryTTF() assumes that font data is to be held by user's hand. */
		s3d::Array<s3d::Byte> fontData = fontBlob.asArray();
		fontAtlas->AddFontFromMemoryTTF(fontData.data(), static_cast<int>(fontData.size()), 24.f, nullptr, glyph);
		ImGui_ImplSiv3d_HoldFontData(fontData);
	}
#endif
}

void CSiv3dMainWindow::imGuiSpineToolDialogue()
{
	if (!m_siv3dSpinePlayer.hasSpineBeenLoaded())return;
	if (m_isSpineToolHidden)return;

	siv3d_imgui_dialogue::ShowSpineTool(m_spineToolDatum);
}

void CSiv3dMainWindow::imGuiHelpDialogue() const
{
	if (!m_isHelpDialogueShown)return;

	siv3d_imgui_dialogue::ShowHelp();
}
