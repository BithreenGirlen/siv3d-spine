

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

		s3d::int32 menuBarHeight = m_siv3dWindowMenu.isVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;
		if (m_pSpinePlayerTexture.get() != nullptr)
		{
			m_pSpinePlayerTexture->clear(s3d::ColorF(0.f, 0.f));
			{
				const s3d::ScopedRenderTarget2D spinePlayerRenderTarget(*m_pSpinePlayerTexture.get());
				const s3d::Transformer2D t(m_siv3dSpinePlayer.calculateTransformMatrix(m_pSpinePlayerTexture->size()));

				m_siv3dSpinePlayer.redraw();
			}

			m_pSpinePlayerTexture->draw(0, menuBarHeight);

			spinePostRendering();
		}

		imGuiSpineParameterDialogue();
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
			{ U"Window", { U"\U000F06D1Hide Spine parameter", U"\U000F0625Show help", U"\U000F18F4Fit base size to current frame", U"Reset base size", U"Disable auto resizing"}}
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
				{ std::bind(&CSiv3dMainWindow::menuOnHideSpineParameter, this), CSiv3dWindowMenu::Restrictive::Yes },
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
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		m_siv3dSpinePlayer.setVisibility(true);
		m_siv3dSpinePlayer.setPause(false);
		m_siv3dSpinePlayer.setTimeScale(1.f);
		m_siv3dSpinePlayer.restartAnimation();
		m_siv3dRecorder.start(s3d::Size(m_pSpinePlayerTexture->width(), m_pSpinePlayerTexture->height()), CSiv3dRecorder::EOutputType::Gif, m_imageFps);
	}
}

void CSiv3dMainWindow::menuOnExportAsVideo()
{
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		m_siv3dSpinePlayer.setVisibility(true);
		m_siv3dSpinePlayer.setPause(false);
		m_siv3dSpinePlayer.setTimeScale(1.f);
		m_siv3dSpinePlayer.restartAnimation();
		m_siv3dRecorder.start(s3d::Size(m_pSpinePlayerTexture->width(), m_pSpinePlayerTexture->height()), CSiv3dRecorder::EOutputType::Video, m_videoFps);
	}
}

void CSiv3dMainWindow::menuOnHideSpineParameter()
{
	m_isSpineParameterHidden ^= true;
	m_siv3dWindowMenu.setLastItemChecked(m_isSpineParameterHidden);
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

void CSiv3dMainWindow::imGuiSpineParameterDialogue()
{
	if (!m_siv3dSpinePlayer.hasSpineBeenLoaded())return;

	if (m_isSpineParameterHidden)return;

#if 0
#define U8_CAST(str) reinterpret_cast<const char*>(u8##str)
#endif

	struct ImGuiComboBox
	{
		unsigned int selectedIndex = 0;

		void update(const std::vector<std::string>& itemNames, const char* comboLabel)
		{
			if (selectedIndex >= itemNames.size())
			{
				selectedIndex = 0;
				return;
			}

			if (ImGui::BeginCombo(comboLabel, itemNames[selectedIndex].c_str()))
			{
				for (size_t i = 0; i < itemNames.size(); ++i)
				{
					bool isSelected = (selectedIndex == i);
					if (ImGui::Selectable(itemNames[i].c_str(), isSelected))
					{
						selectedIndex = static_cast<unsigned int>(i);
					}

					if (isSelected)ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
	};

	struct ImGuiListview
	{
		s3d::Array<bool> checks;
		ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_NoAutoSelect | ImGuiMultiSelectFlags_NoAutoClear | ImGuiMultiSelectFlags_ClearOnEscape;

		void update(const std::vector<std::string>& itemNames, const char* windowLabel)
		{
			if (checks.size() != itemNames.size())
			{
				clear(itemNames);
			}

			ImVec2 childWindowSize = { ImGui::GetWindowWidth() * 3 / 4.f, ImGui::GetFontSize() * (checks.size() / 4 + 2LL) };
			if (ImGui::BeginChild(windowLabel, childWindowSize, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY))
			{
				ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, -1, static_cast<int>(itemNames.size()));
				ImGuiSelectionExternalStorage storage_wrapper;
				storage_wrapper.UserData = (void*)checks.data();
				storage_wrapper.AdapterSetItemSelected = [](ImGuiSelectionExternalStorage* self, int n, bool selected) { bool* array = (bool*)self->UserData; array[n] = selected; };
				storage_wrapper.ApplyRequests(ms_io);
				for (size_t i = 0; i < itemNames.size(); ++i)
				{
					ImGui::SetNextItemSelectionUserData(i);
					ImGui::Checkbox(itemNames[i].c_str(), &checks[i]);
				}

				ms_io = ImGui::EndMultiSelect();
				storage_wrapper.ApplyRequests(ms_io);
			}

			ImGui::EndChild();
		}

		void pickupCheckedItems(const std::vector<std::string>& itemNames, std::vector<std::string>& selectedItems)
		{
			if (itemNames.size() != checks.size())return;

			for (size_t i = 0; i < checks.size(); ++i)
			{
				if (checks[i])
				{
					selectedItems.emplace_back(itemNames[i]);
				}
			}
		}


		void clear(const std::vector<std::string>& itemNames)
		{
			checks = s3d::Array<bool>(itemNames.size(), false);
		}
	};

	const auto HelpMarker = [](const char* desc)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::BeginItemTooltip())
			{
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
		};

	const auto ScrollableSliderInt = [](const char* label, int* v, int v_min, int v_max)
		{
			ImGui::SliderInt(label, v, v_min, v_max);
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
			if (ImGui::IsItemHovered())
			{
				float wheel = ImGui::GetIO().MouseWheel;
				if (wheel > 0 && *v < v_max)
				{
					++(*v);
				}
				else if (wheel < 0 && *v > v_min)
				{
					--(*v);
				}
			}
		};

	const auto ScrollableSliderFloat = [](const char* label, float* v, float v_min, float v_max)
		{
			ImGui::SliderFloat(label, v, v_min, v_max, "%.1f");
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
			if (ImGui::IsItemHovered())
			{
				float wheel = ImGui::GetIO().MouseWheel;
				if (wheel > 0 && *v < v_max)
				{
					++(*v);
				}
				else if (wheel < 0 && *v > v_min)
				{
					--(*v);
				}
			}
		};

	ImGui::Begin("Spine parameter");

	if (ImGui::BeginTabBar("Parameter tabs", ImGuiTabBarFlags_None))
	{
		/* 寸法・座標・尺度 */
		if (ImGui::BeginTabItem("Size/Scale"))
		{
			if (m_pSpinePlayerTexture != nullptr)
			{
				const auto& textureSize = m_pSpinePlayerTexture->size();
				ImGui::Text("Texture size: (%d, %d)", textureSize.x, textureSize.y);
			}

			const s3d::Vector2D<float> baseSize = m_siv3dSpinePlayer.getBaseSize();
			const s3d::Vector2D<float> offset = m_siv3dSpinePlayer.getOffset();

			ImGui::Text("Skeleton size: (%.2f, %.2f)", baseSize.x, baseSize.y);
			ImGui::Text("Offset: (%.2f, %.2f)", offset.x, offset.y);
			ImGui::Text("Skeleton scale: %.2f", m_siv3dSpinePlayer.getSkeletonScale());
			ImGui::Text("Canvas scale: %.2f", m_spineCanvasScale);

			/* 境界矩形 */
			if (ImGui::TreeNode("Slot bounding"))
			{
				const std::vector<std::string>& slotNames = m_siv3dSpinePlayer.getSlotNames();
				static ImGuiComboBox slotsComboBox;
				slotsComboBox.update(slotNames, "Slot##SlotBounding");
				const s3d::Optional<s3d::Vector4D<float>>& slotBounding = m_siv3dSpinePlayer.getCurrentBoundingOfSlot(slotNames[slotsComboBox.selectedIndex]);
				if (!slotBounding)
				{
					ImGui::TextColored(ImVec4{ 1.f, 0.f, 0.f, 1.f }, "Slot not found in this animation.");
				}
				else
				{
					ImGui::Text("Slot bounding: (%.2f, %.2f, %.2f, %.2f)", slotBounding->x, slotBounding->y, slotBounding->x + slotBounding->z, slotBounding->y + slotBounding->w);

					static bool toDrawRect = false;
					ImGui::Checkbox("Draw slot bounding", &toDrawRect);
					if (toDrawRect)
					{
						static constexpr float fMinThickness = 1.f;
						static constexpr float fMaxThickness = 14.f;
						static float fThickness = 2.f;
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
						ScrollableSliderFloat("Thickness", &fThickness, fMinThickness, fMaxThickness);

						static ImVec4 fRectangleColor = ImVec4(240 / 255.f, 240 / 255.f, 240 / 255.f, 1.00f);
						ImGui::SameLine();
						ImGui::ColorEdit4("Colour", (float*)&fRectangleColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
						{
							const s3d::RectF rectF{ slotBounding->x, slotBounding->y, slotBounding->z, slotBounding->w };
							const s3d::ColorF colour(fRectangleColor.x, fRectangleColor.y, fRectangleColor.z, fRectangleColor.w);
							const s3d::Transformer2D t(m_siv3dSpinePlayer.calculateTransformMatrix(m_pSpinePlayerTexture->size()));
							rectF.drawFrame(fThickness, colour);
						}

						/* 特定のスロットに大きさ・位置を合わせる。 */
						if (ImGui::Button("Fit to this slot"))
						{
							/*
							* Todo: 直感的に設定できる設計にする
							*
							* 行っているのは以下の計算:
							* (1)スロットの大きさに合わせ、
							* (2)ワールド座標系を更新し、
							* (3)新しい座標系でのスロット位置を求め、
							* (4)基準位置を更新。
							*/
							m_siv3dSpinePlayer.setBaseSize(slotBounding->z, slotBounding->w);
							m_siv3dSpinePlayer.update(0.f);
							const s3d::Optional<s3d::Vector4D<float>>& updatedSlotBounding = m_siv3dSpinePlayer.getCurrentBoundingOfSlot(slotNames[slotsComboBox.selectedIndex]);
							if (updatedSlotBounding)
							{
								s3d::Vector2D<float> offsetToBe = m_siv3dSpinePlayer.getOffset();
								offsetToBe.x += updatedSlotBounding->x;
								offsetToBe.y += updatedSlotBounding->y;
								m_siv3dSpinePlayer.setOffset(offsetToBe.x, offsetToBe.y);
								m_siv3dSpinePlayer.setBaseSize(slotBounding->z, slotBounding->w);

								m_spineCanvasScale = m_siv3dSpinePlayer.getSkeletonScale();
								resizeWindow();
							}
						}
					}
				}
				ImGui::TreePop();
			}

			ImGui::EndTabItem();
		} /* 寸法・座標・尺度 */

		/* 動作 */
		if (ImGui::BeginTabItem("Animation"))
		{
			const char* pzAnimationName = m_siv3dSpinePlayer.getCurrentAnimationName();
			s3d::Vector4D<float> animationWatch{};
			m_siv3dSpinePlayer.getCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);

			/* 動作名と再生区間 */
			ImGui::SliderFloat(pzAnimationName, &animationWatch.y, animationWatch.z, animationWatch.w, "%0.2f");
			ImGui::Text("Time scale: %.2f", m_siv3dSpinePlayer.getTimeScale());

			const std::vector<std::string>& animationNames = m_siv3dSpinePlayer.getAnimationNames();
			/* 動作指定 */
			if (ImGui::TreeNode("Set animation"))
			{
				static ImGuiComboBox animationComboBox;
				animationComboBox.update(animationNames, "##AnimationToSet");

				if (ImGui::Button("Apply##SetAnimation"))
				{
					m_siv3dSpinePlayer.setAnimationByIndex(animationComboBox.selectedIndex);
				}

				ImGui::TreePop();
			}
			/* 動作予約 */
			if (ImGui::TreeNode("Add tracks"))
			{
				HelpMarker("Adding tracks will overwrite animation state.\n"
					"Clear them before setting another animation.");

				static ImGuiListview animationTracksListView;
				animationTracksListView.update(animationNames, "Tracks to add##AnimationTracksToAdd");

				if (ImGui::Button("Add##AddAnimationTracks"))
				{
					std::vector<std::string> checkedItems;
					animationTracksListView.pickupCheckedItems(animationNames, checkedItems);
					m_siv3dSpinePlayer.addAnimationTracks(checkedItems);
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear##ClearAnimationTracks"))
				{
					animationTracksListView.clear(animationNames);
					m_siv3dSpinePlayer.addAnimationTracks({});
				}

				ImGui::TreePop();
			}

			/* 動作遷移時間 */
			if (ImGui::TreeNode("Mix animations"))
			{
				static ImGuiComboBox fadeOutAnimationComboBox;
				fadeOutAnimationComboBox.update(animationNames, "##AnimationToFadeOut");
				ImGui::SameLine();
				ImGui::Text("Fade out");

				static ImGuiComboBox fadeInAnimationComboBox;
				fadeInAnimationComboBox.update(animationNames, "##AnimationToFadeIn");
				ImGui::SameLine();
				ImGui::Text("Fade in");

				const std::string& fadeOut = animationNames[fadeOutAnimationComboBox.selectedIndex];
				const std::string& fadeIn = animationNames[fadeInAnimationComboBox.selectedIndex];

				float duration = m_siv3dSpinePlayer.getAnimationDuration(fadeOut.c_str());
				if (s3d::NotEqual(duration, 0.f))
				{
					static float mixTime = 0.1f;
					ImGui::SliderFloat("Mix time", &mixTime, 0.f, duration, "%0.2f");

					if (ImGui::Button("Mix##MixAnimations"))
					{
						m_siv3dSpinePlayer.mixAnimations(fadeOut.c_str(), fadeIn.c_str(), mixTime);
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear##ClearMixedAnimations"))
					{
#if defined (SPINE_4_1_OR_LATER) || defined (SPINE_4_2_OR_LATER)
						m_siv3dSpinePlayer.clearMixedAnimation();
#else /* Spine 4.0以前 */
						/* Spine::HashMapの操作手段が存在しない。 */
						m_siv3dSpinePlayer.mixAnimations(fadeOut.c_str(), fadeIn.c_str(), 0.f);
#endif
					}
				}

				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} /* 動作 */

		/* 装い */
		if (ImGui::BeginTabItem("Skin"))
		{
			const std::vector<std::string>& skinNames = m_siv3dSpinePlayer.getSkinNames();
			/* 装い指定 */
			if (ImGui::TreeNode("Set Skin"))
			{
				static ImGuiComboBox skinComboBox;
				skinComboBox.update(skinNames, "##SkinToSet");

				if (ImGui::Button("Apply##SetSkin"))
				{
					m_siv3dSpinePlayer.setSkinByIndex(skinComboBox.selectedIndex);
				}

				ImGui::TreePop();
			}
			/* 装い合成 */
			if (ImGui::TreeNode("Mix skin"))
			{
				HelpMarker("Mixing skins will overwrite the current skin.\n"
					"The state cannnot be gotten back to the original state unless reloaded.");

				static ImGuiListview skinListView;
				skinListView.update(skinNames, "Skins to mix##SkinsToMix");

				if (ImGui::Button("Apply##MixSkins"))
				{
					std::vector<std::string> checkedItems;
					skinListView.pickupCheckedItems(skinNames, checkedItems);
					m_siv3dSpinePlayer.mixSkins(checkedItems);
				}
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} /* 装い */

		/* 槽溝 */
		if (ImGui::BeginTabItem("Slot"))
		{
			const std::vector<std::string>& slotNames = m_siv3dSpinePlayer.getSlotNames();
			/* 描画対象から除外 */
			if (ImGui::TreeNode("Exclude slot"))
			{
				HelpMarker("Checked slots will be excluded from rendering.");

				static ImGuiListview slotListView;
				slotListView.update(slotNames, "Slots to exclude##SlotsToExclude");

				if (ImGui::Button("Apply##ExcludeSlots"))
				{
					std::vector<std::string> checkedItems;
					slotListView.pickupCheckedItems(slotNames, checkedItems);
					m_siv3dSpinePlayer.setSlotsToExclude(checkedItems);
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##ExcludeSlots"))
				{
					slotListView.clear(slotNames);
					m_siv3dSpinePlayer.setSlotsToExclude({});
				}

				ImGui::TreePop();
			}

			/* 挿げ替え */
			if (ImGui::TreeNode("Replace attachment"))
			{
				HelpMarker(
					"This feature is available only when there be slot associated with multiple attachments.\n"
					"Even if it is permitted to replace slot, it does not gurantee the consistency in timeline.");

				/* 滅多に利用機会がないので、非効率なのは承知でこのまま。 */
				const auto& slotAttachmentMap = m_siv3dSpinePlayer.getSlotNamesWithTheirAttachments();
				if (!slotAttachmentMap.empty())
				{
					std::vector<std::string> slotCandidates;
					slotCandidates.reserve(slotAttachmentMap.size());
					for (const auto& slot : slotAttachmentMap)
					{
						slotCandidates.emplace_back(slot.first);
					}

					static ImGuiComboBox slotsComboBox;
					slotsComboBox.update(slotCandidates, "Slot##SlotCandidates");

					const auto& iter = slotAttachmentMap.find(slotCandidates[slotsComboBox.selectedIndex]);
					if (iter != slotAttachmentMap.cend())
					{
						static ImGuiComboBox attachmentComboBox;
						attachmentComboBox.update(iter->second, "Attachment##AssociatesAttachments");

						if (ImGui::Button("Apply##ReplaceAttachment"))
						{
							m_siv3dSpinePlayer.replaceAttachment(
								slotCandidates[slotsComboBox.selectedIndex].c_str(),
								iter->second[attachmentComboBox.selectedIndex].c_str()
							);
						}
					}
				}
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} /* 槽溝 */

		/* 描画 */
		if (ImGui::BeginTabItem("Rendering"))
		{
			bool pma = m_siv3dSpinePlayer.isAlphaPremultiplied();
#if defined(SPINE_4_0) || defined(SPINE_4_1_OR_LATER) || defined(SPINE_4_2_OR_LATER)
			ImGui::BeginDisabled();
#endif
			bool pmaCheckPressed = ImGui::Checkbox("Premultiplied alpha", &pma);
			HelpMarker("For Spine 3.8 and older, PMA should be configured manually.\n"
				"For Spine 4.0 and later, PMA property of atlas page is applied.");
#if defined(SPINE_4_0) || defined(SPINE_4_1_OR_LATER) || defined(SPINE_4_2_OR_LATER)
			ImGui::EndDisabled();
#endif
			if (pmaCheckPressed)
			{
				m_siv3dSpinePlayer.premultiplyAlpha(pma);
			}

			bool toForceBlendModeNormal = m_siv3dSpinePlayer.isBlendModeNormalForced();
			if (ImGui::Checkbox("Force blend-mode-normal", &toForceBlendModeNormal))
			{
				m_siv3dSpinePlayer.forceBlendModeNormal(toForceBlendModeNormal);
			}
			HelpMarker("Force if blend-mode-multiply is not well rendered.");

			bool drawOrder = m_siv3dSpinePlayer.isDrawOrderReversed();
			bool drawOrderConfigureWorthy = m_siv3dSpinePlayer.getNumberOfSpines() > 1;
			if (drawOrderConfigureWorthy)
			{
				if (ImGui::Checkbox("Reverse draw order", &drawOrder))
				{
					m_siv3dSpinePlayer.setDrawOrder(drawOrder);
				}
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::Checkbox("Reverse draw order", &drawOrder);
				ImGui::EndDisabled();
			}

			HelpMarker("Draw order is crutial only when rendering multiple Spines.\n"
				"Be sure to make it appropriate in prior to add animation effect.");

			bool isVisible = m_siv3dSpinePlayer.isVisible();
			if (ImGui::Checkbox("Visible", &isVisible))
			{
				m_siv3dSpinePlayer.setVisibility(isVisible);
			}

			bool isPaused = m_siv3dSpinePlayer.isPaused();
			if (ImGui::Checkbox("Paused", &isPaused))
			{
				m_siv3dSpinePlayer.setPause(isPaused);
			}

			/* 統計 */
			if (ImGui::TreeNode("Statistics"))
			{
				/* Visibleのチェックを外した時との差分が、Spineの描画に費やしている命令数。 */
				const s3d::ProfilerStat& profilerStat = s3d::Profiler::GetStat();
				ImGui::Text("Draw calls: %lu", profilerStat.drawCalls);
				ImGui::Text("Triangle count: %lu", profilerStat.triangleCount);

				bool isVsyncEnabled = s3d::Graphics::IsVSyncEnabled();
				if (ImGui::Checkbox("VSync", &isVsyncEnabled))
				{
					s3d::Graphics::SetVSyncEnabled(isVsyncEnabled);
				}
				ImGui::Text("FPS: %d", s3d::Profiler::FPS());

				ImGui::TreePop();
			}

			ImGui::EndTabItem();
		} /* 描画 */

		/* Spineに関係しない、その他設定。 */
		if (ImGui::BeginTabItem("Misc."))
		{
			/* 書き出しFPS */
			if (ImGui::TreeNode("Export"))
			{
				static constexpr int MinFps = 15;
				static constexpr int MaxImageFps = 60;
				static constexpr int MaxVideoFps = 60;

				if constexpr (sizeof(s3d::int32) == sizeof(int))
				{
					ScrollableSliderInt("GIF", &m_imageFps, MinFps, MaxImageFps);
					HelpMarker("GIF delay is defined in 10ms increments.\n Mind that fractional part will be discarded.");
					ScrollableSliderInt("Video", &m_videoFps, MinFps, MaxVideoFps);
				}
				else
				{
					static int imageFps = m_imageFps;
					static int videoFps = m_videoFps;
					ScrollableSliderInt("GIF", &imageFps, MinFps, MaxImageFps);
					ScrollableSliderInt("Video", &videoFps, MinFps, MaxVideoFps);
					m_imageFps = static_cast<s3d::int32>(imageFps);
					m_videoFps = static_cast<s3d::int32>(videoFps);
				}

				ImGui::TreePop();
			}

			/* ImGui字体大きさ変更 */
			if (ImGui::TreeNode("Font size"))
			{
				static constexpr float MinFontSize = 16.f;
				static constexpr float MaxFontSize = 48.f;
				float fontSize = ImGui::GetFontSize();
				float fontSizeValue = fontSize;
				ScrollableSliderFloat("Font size", &fontSizeValue, MinFontSize, MaxFontSize);
				if (s3d::NotEqual(fontSize, fontSizeValue))
				{
					ImGuiStyle& style = ImGui::GetStyle();
					style._NextFrameFontSizeBase = fontSizeValue;
				}

				ImGui::TreePop();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void CSiv3dMainWindow::imGuiHelpDialogue() const
{
	if (!m_isHelpDialogueShown)return;

	struct MouseHelp
	{
		enum
		{
			Input,
			Description,
			kMax
		};
	};
	static constexpr const char* const mouseHelps[][MouseHelp::kMax] =
	{
		{"L-drag", "Move view-point"},
		{"R + L-click", "Switch animation"},
		{"Wheel", "Scale up/down window"},
		{"Ctrl + wheel", "Zoom in/out"},
		{"L + wheel", "Speed up/down animation"},
		{"M-click", "Reset zoom, speed, offset."},
		{"R + M-click", "Show/hide window's border"},
		{"Ctrl + L-drag", "Move borderless window"},
	};

	static float inputTextWidth = 0.f;
	if (inputTextWidth == 0.f)
	{
		for (const auto& mouseHelp : mouseHelps)
		{
			const auto& textSize = ImGui::CalcTextSize(reinterpret_cast<const char*>(mouseHelp[MouseHelp::Input]));
			inputTextWidth = s3d::Max(inputTextWidth, textSize.x);
		}
	}

	ImGui::Begin("Help");

	ImGui::SeparatorText("Mouse functions:");
	ImGui::Indent();
	for (const auto& mouseHelp : mouseHelps)
	{
		ImGui::BulletText(mouseHelp[MouseHelp::Input]);
		ImGui::SameLine(inputTextWidth * 2.f);
		ImGui::Text(": %s", mouseHelp[MouseHelp::Description]);
	}
	ImGui::Unindent();

	ImGui::SeparatorText("Keyboard functions:");
	ImGui::Indent();
	ImGui::BulletText("M: Show/hide window menu.");
	ImGui::Unindent();

	ImGui::SeparatorText("How to load");
	ImGui::Indent();
	ImGui::Text("1. Click file menu \"File -> Open file\".\n2. Select exported atlas file.\n3. Select pair skeleton file.");
	ImGui::Unindent();

	ImGui::End();
}
