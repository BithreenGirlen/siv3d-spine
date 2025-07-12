

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

	InitialiseMenuBar();
}

CSiv3dMainWindow::~CSiv3dMainWindow()
{

}

void CSiv3dMainWindow::Display()
{
#if 0
	s3d::Graphics::SetVSyncEnabled(false);
#endif

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsLight();
	ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.875f;

	ImGui_ImplSiv3d_Init();

	SetEmbeddedFontForImgui();

	while (s3d::System::Update())
	{
		ImGui_ImplSiv3d_NewFrame();
		ImGui::NewFrame();

		ImGuiSpineParameterDialogue();
		ImGuiHelpDialogue();

		m_siv3dWindowMenu.Update();

		HandleMouseEvent();
		HandleKeyboardEvent();

		m_siv3dSpinePlayer.Update(static_cast<float>(s3d::Scene::DeltaTime()));

		s3d::int32 menuBarHeight = m_siv3dWindowMenu.IsVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;
		if (m_pSpinePlayerTexture.get() != nullptr)
		{
			m_pSpinePlayerTexture->clear(s3d::ColorF(0.f, 0.f));
			{
				const s3d::ScopedRenderTarget2D spinePlayerRenderTarget(*m_pSpinePlayerTexture.get());
				m_siv3dSpinePlayer.Redraw();
			}

			m_pSpinePlayerTexture->draw(0, menuBarHeight);

			SpinePostRendering();
		}

		ImGui::Render();
		ImGui_ImplSiv3d_RenderDrawData();

		/* 操作欄を最前面に。 */
		m_siv3dWindowMenu.Draw();
	}

	ImGui_ImplSiv3d_Shutdown();
}

void CSiv3dMainWindow::InitialiseMenuBar()
{
	if (!m_siv3dWindowMenu.HasBeenInitialised())
	{
		const s3d::Array<std::pair<s3d::String, s3d::Array<s3d::String>>> menuItems
		{
			{ U"File", { U"Open file"} },
			{ U"Export", { U"Snap as Webp", U"Export as GIF", U"Export as video"} },
			{ U"Window", { U"Hide Spine parameter", U"Show help"}}
		};

		const s3d::Array<s3d::Array<CSiv3dWindowMenu::ItemProprty>> menuItemProperties
		{
			{
				{ std::bind(&CSiv3dMainWindow::MenuOnOpenFile, this), CSiv3dWindowMenu::Restrictive::No }
			},
			{
				{ std::bind(&CSiv3dMainWindow::MenuOnSnapImage, this), CSiv3dWindowMenu::Restrictive::Yes },
				{ std::bind(&CSiv3dMainWindow::MenuOnExportAsGif, this), CSiv3dWindowMenu::Restrictive::Yes },
				{ std::bind(&CSiv3dMainWindow::MenuOnExportAsVideo, this), CSiv3dWindowMenu::Restrictive::Yes }
			},
			{
				{ std::bind(&CSiv3dMainWindow::MenuOnHideSpineParameter, this), CSiv3dWindowMenu::Restrictive::Yes },
				{ std::bind(&CSiv3dMainWindow::MenuOnShowHelp, this), CSiv3dWindowMenu::Restrictive::No }
			}
		};

		m_siv3dWindowMenu.Initialise(menuItems, menuItemProperties);
	}
}

void CSiv3dMainWindow::MenuOnOpenFile()
{
	const s3d::Array<s3d::String> atlasCandidates = { U"atlas" , U"atlas.txt" };
	auto selectedAtlas = s3d::Dialog::OpenFile({ { U"Atlas file", { atlasCandidates } }, { U"All files", { U"*" } } }, U"", U"Select atlas");
	if (!selectedAtlas.has_value())return;

	const s3d::Array<s3d::String> skelCandidates = { U"skel", U"bin", U"bytes", U"json" };
	auto selectedSkeleton = s3d::Dialog::OpenFile({ { U"Skeleton file", skelCandidates }, { U"All files", { U"*" } } }, U"", U"Select skeleton");
	if (!selectedSkeleton.has_value())return;

	const auto IsBinarySkeleton = [&](const s3d::String& str)
		-> bool
		{
			const s3d::Array<s3d::String> binaryCandidates = { U"skel", U"bin", U"bytes" };
			for (const auto& binaryCandidate : binaryCandidates)
			{
				if (str.contains(binaryCandidate))return true;
			}
			return false;
		};
	bool isBinarySkel = IsBinarySkeleton(s3d::FileSystem::FileName(selectedSkeleton.value()));

	std::vector<std::string> atlasPaths;
	std::vector<std::string> skelPaths;

	atlasPaths.push_back(s3d::Unicode::ToUTF8(selectedAtlas.value()));
	skelPaths.push_back(s3d::Unicode::ToUTF8(selectedSkeleton.value()));

	m_siv3dSpinePlayer.LoadSpineFromFile(atlasPaths, skelPaths, isBinarySkel);

	m_siv3dWindowMenu.UpdateRestrictiveItemState(m_siv3dSpinePlayer.HasSpineBeenLoaded());
	if (m_siv3dSpinePlayer.HasSpineBeenLoaded())
	{
		s3d::Window::SetTitle(s3d::FileSystem::BaseName(selectedAtlas.value()));
	}
	ResizeWindow();
}

void CSiv3dMainWindow::MenuOnSnapImage()
{
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		s3d::Image image;
		m_pSpinePlayerTexture->readAsImage(image);

		s3d::FilePath filePath = BuildExportFilePath();
		{
			s3d::Vector4D<float> animationWatch{};
			m_siv3dSpinePlayer.GetCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);

			using namespace s3d;
			const auto& formatted = U"_{:.2f}"_fmt(animationWatch.y);
			filePath += formatted;
		}

		filePath += U".webp";
		image.saveWebP(filePath);
	}
}

void CSiv3dMainWindow::MenuOnExportAsGif()
{
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		m_siv3dSpinePlayer.RestartAnimation();
		m_siv3dRecorder.Start(s3d::Size(m_pSpinePlayerTexture->width(), m_pSpinePlayerTexture->height()), CSiv3dRecorder::EOutputType::Gif, m_imageFps);
	}
}

void CSiv3dMainWindow::MenuOnExportAsVideo()
{
	if (m_pSpinePlayerTexture.get() != nullptr)
	{
		m_siv3dSpinePlayer.RestartAnimation();
		m_siv3dRecorder.Start(s3d::Size(m_pSpinePlayerTexture->width(), m_pSpinePlayerTexture->height()), CSiv3dRecorder::EOutputType::Video, m_videoFps);
	}
}

void CSiv3dMainWindow::MenuOnHideSpineParameter()
{
	m_isSpineParameterHidden ^= true;
	m_siv3dWindowMenu.SetLastItemChecked(m_isSpineParameterHidden);
}

void CSiv3dMainWindow::MenuOnShowHelp()
{
	m_isHelpDialogueShown ^= true;
	m_siv3dWindowMenu.SetLastItemChecked(m_isHelpDialogueShown);
}
/* マウス入力処理 */
void CSiv3dMainWindow::HandleMouseEvent()
{
	if (m_siv3dRecorder.IsUnderRecording())return;

	const auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse)return;

	if (s3d::MouseL.pressed())
	{
		if (s3d::KeyLControl.pressed() && s3d::Window::GetStyle() == s3d::WindowStyle::Frameless)
		{
			/* 枠無しウィンドウ移動 */
			s3d::Point mouseDelta = s3d::Cursor::ScreenDelta();
			s3d::Rect windowRect = s3d::Window::GetState().bounds;

			s3d::Point windowPosToBe = s3d::Point{ windowRect.x + mouseDelta.x, windowRect.y + mouseDelta.y };
			s3d::Window::SetPos(windowPosToBe);
		}
		else
		{
			if (!s3d::Window::GetState().sizeMove)
			{
				/* 視点移動 */
				s3d::Point mouseDelta = s3d::Cursor::Delta();
				m_siv3dSpinePlayer.MoveViewPoint(-mouseDelta.x, -mouseDelta.y);
			}

		}
	}
	else if (s3d::MouseL.up())
	{
		if (s3d::MouseR.pressed())
		{
			m_siv3dSpinePlayer.ShiftAnimation();
		}
		else
		{
			CheckRenderTextureSize();
		}
	}
	else if (s3d::MouseM.up())
	{
		if (s3d::MouseR.pressed())
		{
			ToggleWindowBorderStyle();
		}
		else
		{
			/* 視点・速度・尺度初期化 */
			m_siv3dSpinePlayer.ResetScale();
			ResizeWindow();
		}
	}

	if (s3d::Mouse::Wheel())
	{
		if (s3d::MouseL.pressed())
		{
			/* 加減速 */
			m_siv3dSpinePlayer.RescaleTime(s3d::Mouse::Wheel() > 0);
		}
		else if (!s3d::MouseR.pressed())
		{
			/* 拡縮 */
			m_siv3dSpinePlayer.RescaleSkeleton(s3d::Mouse::Wheel() > 0);
			if (!s3d::KeyLControl.pressed())
			{
				m_siv3dSpinePlayer.RescaleCanvas(s3d::Mouse::Wheel() > 0);
				ResizeWindow();
			}
		}
	}
}
/* キーボード入力処理 */
void CSiv3dMainWindow::HandleKeyboardEvent()
{
	if (m_siv3dRecorder.IsUnderRecording())return;

	const auto& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard)return;

	if (s3d::KeyM.up())
	{
		/* メニュー消去・表示。 */
		m_siv3dWindowMenu.SetVisibility(!m_siv3dWindowMenu.IsVisible());
		ResizeWindow();
	}
	else if (s3d::KeyA.up())
	{
		m_siv3dSpinePlayer.TogglePma();
	}
	else if (s3d::KeyB.up())
	{
		m_siv3dSpinePlayer.ToggleBlendModeAdoption();
	}
	else if (s3d::KeyS.up())
	{
		m_siv3dSpinePlayer.ShiftSkin();
	}
}
/* 窓寸法変更 */
void CSiv3dMainWindow::ResizeWindow()
{
	if (!m_siv3dSpinePlayer.HasSpineBeenLoaded() || m_siv3dRecorder.IsUnderRecording())return;

	s3d::Vector2D<float> fCanvasSize = m_siv3dSpinePlayer.GetBaseSize();
	float fScale = m_siv3dSpinePlayer.GetCanvasScale();

	s3d::int32 iClientWidth = static_cast<s3d::int32>(fCanvasSize.x * fScale);
	s3d::int32 iClientHeight = static_cast<s3d::int32>(fCanvasSize.y * fScale);

	/* メニュー表示時はクライアント領域を高さ分大きく取り、且つ、他の描画対象物の描画開始位置を高さ分下げる。*/
	s3d::int32 menuBarHeight = m_siv3dWindowMenu.IsVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;

	/* 書き出しファイルの寸法を解像度内に抑えるため上限を設ける。 */
	const auto monitorInfo = s3d::System::GetCurrentMonitor();
	iClientWidth = s3d::Min(iClientWidth, monitorInfo.displayRect.w);
	iClientHeight = s3d::Min(iClientHeight, monitorInfo.displayRect.h);

	s3d::Window::ResizeActual(iClientWidth, iClientHeight + menuBarHeight, s3d::YesNo<s3d::Centering_tag>::No);

	CheckRenderTextureSize();
}
/* Spine描画先紋理要再作成確認 */
void CSiv3dMainWindow::CheckRenderTextureSize()
{
	if (!m_siv3dSpinePlayer.HasSpineBeenLoaded()) return;

	s3d::int32 menuBarHeight = m_siv3dWindowMenu.IsVisible() ? s3d::SimpleMenuBar::MenuBarHeight : 0;
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
		m_siv3dSpinePlayer.OnResize(textureSize);
	}
}
/* 窓枠表示・消去 */
void CSiv3dMainWindow::ToggleWindowBorderStyle()
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
void CSiv3dMainWindow::SpinePostRendering()
{
	if (m_siv3dRecorder.IsUnderRecording())
	{
		s3d::Vector4D<float> animationWatch{};
		m_siv3dSpinePlayer.GetCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);
		/* 一周し終わったら書き出し。 */
		if (animationWatch.x > animationWatch.w)
		{
			s3d::FilePath filePath = BuildExportFilePath();
			m_siv3dRecorder.End(filePath);
		}
		else
		{
			s3d::Graphics2D::Flush();
			m_siv3dRecorder.CommitFrame(*m_pSpinePlayerTexture.get());
		}
	}
}

s3d::FilePath CSiv3dMainWindow::BuildExportFilePath()
{
	const s3d::FilePath& moduleDirectory = s3d::FileSystem::ParentPath(s3d::FileSystem::ModulePath());
	const s3d::FilePath& saveFolderPath = moduleDirectory + s3d::Window::GetTitle() + U'/';

	const char* pzAnimationName = m_siv3dSpinePlayer.GetCurrentAnimationName();
	s3d::String fileName = pzAnimationName == nullptr ? U"" : s3d::Unicode::FromUTF8(pzAnimationName);

	return saveFolderPath + fileName;
}
/* ImGui用書体設定 */
void CSiv3dMainWindow::SetEmbeddedFontForImgui() const
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

void CSiv3dMainWindow::ImGuiSpineParameterDialogue()
{
	if (!m_siv3dSpinePlayer.HasSpineBeenLoaded())return;

	if (m_isSpineParameterHidden)return;

#if 0
#define U8_CAST(str) reinterpret_cast<const char*>(u8##str)
#endif

	struct ImGuiComboBox
	{
		unsigned int selectedIndex = 0;

		void Update(const std::vector<std::string>& itemNames, const char* comboLabel)
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

		void Update(const std::vector<std::string>& itemNames, const char* windowLabel)
		{
			if (checks.size() != itemNames.size())
			{
				checks = s3d::Array<bool>(itemNames.size(), false);
			}

			ImVec2 childWindowSize = { ImGui::GetWindowWidth() * 3 / 4.f, ImGui::GetFontSize() * (checks.size() + 2LL) };
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

		void PickupCheckedItems(const std::vector<std::string>& itemNames, std::vector<std::string>& selectedItems)
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
	};

	ImGui::Begin("Spine parameter/manipulator");

	/* 寸法・座標・拡縮度 */
	if (ImGui::CollapsingHeader("Size/Scale"))
	{
		if (m_pSpinePlayerTexture.get() != nullptr)
		{
			const auto& textureSize = m_pSpinePlayerTexture->size();
			ImGui::Text("Texture size: (%d, %d)", textureSize.x, textureSize.y);
		}

		s3d::Vector2D<float> baseSize = m_siv3dSpinePlayer.GetBaseSize();
		s3d::Vector2D<float> offset = m_siv3dSpinePlayer.GetOffset();

		ImGui::Text("Skeleton size: (%.2f, %.2f)", baseSize.x, baseSize.y);
		ImGui::Text("Offset: (%.2f, %.2f)", offset.x, offset.y);
		ImGui::Text("Skeleton scale: %.2f", m_siv3dSpinePlayer.GetSkeletonScale());
		ImGui::Text("Canvas scale: %.2f", m_siv3dSpinePlayer.GetCanvasScale());
	}

	/* 動作名・動作指定・動作合成 */
	if (ImGui::CollapsingHeader("Animation"))
	{
		const char* pzAnimationName = m_siv3dSpinePlayer.GetCurrentAnimationName();
		s3d::Vector4D<float> animationWatch{};
		m_siv3dSpinePlayer.GetCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);

		ImGui::SliderFloat(pzAnimationName, &animationWatch.y, animationWatch.z, animationWatch.w, "%0.2f");
		ImGui::Text("Time scale: %.2f", m_siv3dSpinePlayer.GetTimeScale());

		const std::vector<std::string>& animationNames = m_siv3dSpinePlayer.GetAnimationNames();
		/* 動作指定 */
		if (ImGui::TreeNode("Set animation"))
		{
			static ImGuiComboBox animationComboBox;
			animationComboBox.Update(animationNames, "##AnimationToSet");

			if (ImGui::Button("Apply##SetAnimation"))
			{
				m_siv3dSpinePlayer.SetAnimationByIndex(animationComboBox.selectedIndex);
			}

			ImGui::TreePop();
		}
		/* 動作合成 */
		if (ImGui::TreeNode("Mix animation"))
		{
			ImGui::BulletText("Caution: Mixing animations will overwrite animation state.");
			ImGui::BulletText("Make sure there be no conflict as for attachments.");

			static ImGuiListview animationsListView;
			animationsListView.Update(animationNames, "Animation to mix##AnimationsToMix");

			if (ImGui::Button("Apply##MixAnimations"))
			{
				std::vector<std::string> checkedItems;
				animationsListView.PickupCheckedItems(animationNames, checkedItems);
				m_siv3dSpinePlayer.MixAnimations(checkedItems);
			}

			ImGui::TreePop();
		}
	}

	/* 装い指定・合成 */
	if (ImGui::CollapsingHeader("Skin"))
	{
		const std::vector<std::string>& skinNames = m_siv3dSpinePlayer.GetSkinNames();
		/* 装い指定 */
		if (ImGui::TreeNode("Set Skin"))
		{
			static ImGuiComboBox skinComboBox;
			skinComboBox.Update(skinNames, "##SkinToSet");

			if (ImGui::Button("Apply##SetSkin"))
			{
				m_siv3dSpinePlayer.SetSkinByIndex(skinComboBox.selectedIndex);
			}

			ImGui::TreePop();
		}
		/* 装い合成 */
		if (ImGui::TreeNode("Mix skin"))
		{
			ImGui::BulletText("Caution: Mixing skins will overwrite animation state.");

			static ImGuiListview skinListView;
			skinListView.Update(skinNames, "Skins to mix##SkinsToMix");

			if (ImGui::Button("Apply##MixSkins"))
			{
				std::vector<std::string> checkedItems;
				skinListView.PickupCheckedItems(skinNames, checkedItems);
				m_siv3dSpinePlayer.MixSkins(checkedItems);
			}
			ImGui::TreePop();
		}
	}

	/* 槽溝 */
	if (ImGui::CollapsingHeader("Slot"))
	{
		const std::vector<std::string>& slotNames = m_siv3dSpinePlayer.GetSlotNames();
		/* 描画対象から除外 */
		if (ImGui::TreeNode("Exclude slot"))
		{
			ImGui::BulletText("Checked attachments will be excluded from rendering.\n");

			static ImGuiListview slotListView;
			slotListView.Update(slotNames, "Slots to exclude##SlotsToExclude");

			if (ImGui::Button("Apply##ExcludeSlots"))
			{
				std::vector<std::string> checkedItems;
				slotListView.PickupCheckedItems(slotNames, checkedItems);
				m_siv3dSpinePlayer.SetSlotsToExclude(checkedItems);
			}

			ImGui::TreePop();
		}

		/* 挿げ替え */
		if (ImGui::TreeNode("Replace attachment"))
		{
			ImGui::BulletText("This feature is available only when\n there be slot associated with multiple attachments.\n");
			ImGui::BulletText("Even if it is permitted to replace slot,\n it does not guarantee consistency in timeline.");

			/* 滅多に利用機会がないので、非効率なのは承知でこのまま。 */
			const auto& slotAttachmentMap = m_siv3dSpinePlayer.GetSlotNamesWithTheirAttachments();
			if (!slotAttachmentMap.empty())
			{
				std::vector<std::string> slotCandidates;
				slotCandidates.reserve(slotAttachmentMap.size());
				for (const auto& slot : slotAttachmentMap)
				{
					slotCandidates.emplace_back(slot.first);
				}

				static ImGuiComboBox slotsComboBox;
				slotsComboBox.Update(slotCandidates, "Slot##SlotCandidates");

				const auto& iter = slotAttachmentMap.find(slotCandidates[slotsComboBox.selectedIndex]);
				if (iter != slotAttachmentMap.cend())
				{
					static ImGuiComboBox attachmentComboBox;
					attachmentComboBox.Update(iter->second, "Attachment##AssociatesAttachments");

					if (ImGui::Button("Apply##ReplaceAttachment"))
					{
						m_siv3dSpinePlayer.ReplaceAttachment(
							slotCandidates[slotsComboBox.selectedIndex].c_str(),
							iter->second[attachmentComboBox.selectedIndex].c_str()
						);
					}
				}
			}
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Others"))
	{
		ImGui::SeparatorText("Premultiplied alpha");

		ImGui::BulletText("For Spine 3.8 and older, PMA should be configured by user hand.");
		ImGui::BulletText("For Spine 4.0 and later, PMA property of atlas page is applied.");
#if !defined(SPINE_4_0) && !defined(SPINE_4_1_OR_LATER) && !defined(SPINE_4_2_OR_LATER)
		bool pma = m_siv3dSpinePlayer.IsAlphaPremultiplied();
		ImGui::Checkbox("Alpha premultiplied", &pma);
		m_siv3dSpinePlayer.PremultiplyAlpha(pma);
#endif
		ImGui::SeparatorText("Blend-mode");

		ImGui::BulletText("Blend-mode-multiply will not be represented intentionally\n if alpha were not premultiplied when exported.");
		bool toForceBlendModeNormal = m_siv3dSpinePlayer.IsBlendModeNormalForced();
		ImGui::Checkbox("To force blend-mode-normal", &toForceBlendModeNormal);
		m_siv3dSpinePlayer.ForceBlendModeNormal(toForceBlendModeNormal);

		if (m_siv3dSpinePlayer.GetNumberOfSpines() > 1)
		{
			ImGui::SeparatorText("Draw order");

			bool drawOrder = m_siv3dSpinePlayer.IsDrawOrderReversed();
			ImGui::Checkbox("Reverse draw order", &drawOrder);
			m_siv3dSpinePlayer.SetDrawOrder(drawOrder);
		}

		if (ImGui::TreeNode("Export FPS"))
		{
			ImGui::BulletText("Caution: GIF delay is defined in 10ms increments.\n Mind that fractional part will be discarded.");

			constexpr int minFps = 15;
			constexpr int maxImageFps = 60;
			constexpr int maxVideoFps = 120;
			if constexpr (sizeof(s3d::int32) == sizeof(int))
			{
				ImGui::SliderInt("GIF", &m_imageFps, minFps, maxImageFps);
				ImGui::SliderInt("Video", &m_videoFps, minFps, maxVideoFps);
			}
			else
			{
				static int imageFps = m_imageFps;
				static int videoFps = m_videoFps;
				ImGui::SliderInt("GIF", &imageFps, minFps, maxImageFps);
				ImGui::SliderInt("Video", &videoFps, minFps, maxVideoFps);
				m_imageFps = static_cast<s3d::int32>(imageFps);
				m_videoFps = static_cast<s3d::int32>(videoFps);
			}

			ImGui::TreePop();
		}
	}

	ImGui::End();
}

void CSiv3dMainWindow::ImGuiHelpDialogue() const
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
	constexpr const char* const mouseHelps[][MouseHelp::kMax] =
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

	ImGui::Begin("Help");

	float fWindowWidth = ImGui::GetWindowWidth();

	ImGui::SeparatorText("Mouse functions:");
	ImGui::Indent();
	for (size_t i = 0; i < sizeof(mouseHelps) / sizeof(mouseHelps[0]); ++i)
	{
		ImGui::BulletText(mouseHelps[i][MouseHelp::Input]);
		ImGui::SameLine(fWindowWidth * 7 / 16.f);
		ImGui::Text(": %s", mouseHelps[i][MouseHelp::Description]);
	}
	ImGui::Unindent();

	ImGui::SeparatorText("Keyboard functions:");
	ImGui::Indent();
	ImGui::BulletText("M: Show/hide window menu.");
#if !defined(SPINE_4_0) && !defined(SPINE_4_1_OR_LATER) && !defined(SPINE_4_2_OR_LATER)
	ImGui::BulletText("A: Toggle PMA");
#endif
	ImGui::Unindent();

	ImGui::SeparatorText("How to load");
	ImGui::Indent();
	ImGui::Text("1. Click file menu \"File -> Open file\".\n2. Select exported atlas file.\n3. Select pair skeleton file.");
	ImGui::Unindent();

	ImGui::End();
}
