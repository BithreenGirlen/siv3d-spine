

#include "siv3d_imgui_dialogue.h"

#include <imgui.h>

#if 0
	#define U8_CAST(str) reinterpret_cast<const char*>(u8##str)
#endif

namespace siv3d_imgui_dialogue
{
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

			if (ImGui::BeginCombo(comboLabel, itemNames[selectedIndex].c_str(), ImGuiComboFlags_HeightLarge))
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
	/// @brief 補助説明表示
	static void HelpMarker(const char* desc)
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
	/// @brief ホイール回転で加減可能な整数スライダ
	static void ScrollableSliderInt(const char* label, int* v, int v_min, int v_max)
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
	/// @brief ホイール回転で加減可能な浮動小数スライダ
	static void ScrollableSliderFloat(const char* label, float* v, float v_min, float v_max)
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

	/// @brief 境界矩形描画器
	struct Siv3dImGuiBoundsRenderer
	{
		static constexpr float MinThickness = 1.f;
		static constexpr float MaxThickness = 14.f;

		float fThickness = 2.f;
		ImVec4 fRectangleColor = ImVec4(240 / 255.f, 240 / 255.f, 240 / 255.f, 1.00f);
		bool toDrawRect = false;

		void render(const s3d::Vector4D<float>& bounds, const s3d::Mat3x2& transformMatrix, const s3d::Point &offset)
		{
			ImGui::Text("Bounds: (%.2f, %.2f, %.2f, %.2f)", bounds.x, bounds.y, bounds.x + bounds.z, bounds.y + bounds.w);
			ImGui::Checkbox("Draw rectangle", &toDrawRect);
			if (toDrawRect)
			{
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
				ScrollableSliderFloat("Thickness", &fThickness, MinThickness, MaxThickness);
				ImGui::SameLine();
				ImGui::ColorEdit4("Colour", (float*)&fRectangleColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);

				const s3d::RectF rectF{ bounds.x + offset.x, bounds.y + offset.y, bounds.z, bounds.w };
				const s3d::ColorF colour(fRectangleColor.x, fRectangleColor.y, fRectangleColor.z, fRectangleColor.w);
				const s3d::Transformer2D t(transformMatrix);
				rectF.drawFrame(fThickness, colour);
			}
		}
	};
}

void siv3d_imgui_dialogue::ShowSpineTool(SpineToolDatum& spineToolDatum)
{
	CSiv3dSpinePlayer& siv3dSpinePlayer = spineToolDatum.siv3dSpinePlayer;
	std::unique_ptr<s3d::RenderTexture>& pSpinePlayerTexture = spineToolDatum.spinePlayerTexture;
	float& spineCanvasScale = spineToolDatum.spineCanvasScale;
	bool& isWireframeMode = spineToolDatum.isWireFrameMode;
	s3d::int32& imageFps = spineToolDatum.imageFps;
	s3d::int32& videoFps = spineToolDatum.videoFps;

	ImGui::Begin("Spine tool");

	if (ImGui::BeginTabBar("Tool tabs", ImGuiTabBarFlags_None))
	{
		/* 寸法・座標・尺度 */
		if (ImGui::BeginTabItem("Size/Scale"))
		{
			if (pSpinePlayerTexture != nullptr)
			{
				const auto& textureSize = pSpinePlayerTexture->size();
				ImGui::Text("Texture size: (%d, %d)", textureSize.x, textureSize.y);
			}

			const s3d::Vector2D<float> baseSize = siv3dSpinePlayer.getBaseSize();
			const s3d::Vector2D<float> offset = siv3dSpinePlayer.getOffset();

			ImGui::Text("Skeleton size: (%.2f, %.2f)", baseSize.x, baseSize.y);
			ImGui::Text("Offset: (%.2f, %.2f)", offset.x, offset.y);
			ImGui::Text("Skeleton scale: %.2f", siv3dSpinePlayer.getSkeletonScale());
			ImGui::Text("Canvas scale: %.2f", spineCanvasScale);

			/* 境界矩形算出・枠表示 */
			if (ImGui::TreeNode("Bounding box"))
			{
				const s3d::Mat3x2 transfromMatrix = siv3dSpinePlayer.calculateTransformMatrix(pSpinePlayerTexture->size());
				/* 特定スロットの境界矩形 */
				if (ImGui::TreeNode("Slot"))
				{
					const std::vector<std::string>& slotNames = siv3dSpinePlayer.getSlotNames();
					static ImGuiComboBox slotsComboBox;
					slotsComboBox.update(slotNames, "Slot##SlotBoundingBox");

					const s3d::Optional<s3d::Vector4D<float>>& slotBoundingBox = siv3dSpinePlayer.getCurrentBoundingBoxOfSlot(slotNames[slotsComboBox.selectedIndex]);
					if (!slotBoundingBox)
					{
						ImGui::TextColored(ImVec4{ 1.f, 0.f, 0.f, 1.f }, "Slot not found in this animation.");
					}
					else
					{
						static Siv3dImGuiBoundsRenderer slotBoundsRenderer;
						slotBoundsRenderer.render(slotBoundingBox.value(), transfromMatrix, spineToolDatum.spineRenderPosition);
						if (slotBoundsRenderer.toDrawRect)
						{
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
								siv3dSpinePlayer.setBaseSize(slotBoundingBox->z, slotBoundingBox->w);
								siv3dSpinePlayer.update(0.f);
								const s3d::Optional<s3d::Vector4D<float>>& updatedSlotBoundingBox = siv3dSpinePlayer.getCurrentBoundingBoxOfSlot(slotNames[slotsComboBox.selectedIndex]);
								if (updatedSlotBoundingBox)
								{
									s3d::Vector2D<float> offsetToBe = siv3dSpinePlayer.getOffset();
									offsetToBe.x += updatedSlotBoundingBox->x;
									offsetToBe.y += updatedSlotBoundingBox->y;
									siv3dSpinePlayer.setOffset(offsetToBe.x, offsetToBe.y);
									siv3dSpinePlayer.setBaseSize(slotBoundingBox->z, slotBoundingBox->w);

									spineCanvasScale = siv3dSpinePlayer.getSkeletonScale();
									spineToolDatum.isWindowToBeResized = true;
								}
							}
						}
					}

					ImGui::TreePop();
				}

				/* 全体の境界矩形 */
				if (ImGui::TreeNode("Whole"))
				{
					s3d::Vector4D<float> wholeBounding = siv3dSpinePlayer.getCurrentBoundingBox();
					static Siv3dImGuiBoundsRenderer wholeBoundsRenderer;
					wholeBoundsRenderer.render(wholeBounding, transfromMatrix, spineToolDatum.spineRenderPosition);

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			ImGui::EndTabItem();
		} /* 寸法・座標・尺度 */

		/* 動作 */
		if (ImGui::BeginTabItem("Animation"))
		{
			const char* pzAnimationName = siv3dSpinePlayer.getCurrentAnimationName();
			s3d::Vector4D<float> animationWatch{};
			siv3dSpinePlayer.getCurrentAnimationTime(&animationWatch.x, &animationWatch.y, &animationWatch.z, &animationWatch.w);

			/* 動作名と再生区間 */
			if (ImGui::SliderFloat(pzAnimationName, &animationWatch.y, animationWatch.z, animationWatch.w, "%0.2f"))
			{
				/* 再生位置変更 */
				siv3dSpinePlayer.setCurrentAnimationTime(animationWatch.y);
			}
			ImGui::Text("Time scale: %.2f", siv3dSpinePlayer.getTimeScale());

			const std::vector<std::string>& animationNames = siv3dSpinePlayer.getAnimationNames();
			/* 動作指定 */
			if (ImGui::TreeNode("Set animation"))
			{
				static ImGuiComboBox animationComboBox;
				animationComboBox.update(animationNames, "##AnimationToSet");

				if (ImGui::Button("Apply##SetAnimation"))
				{
					siv3dSpinePlayer.setAnimationByIndex(animationComboBox.selectedIndex);
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
					siv3dSpinePlayer.addAnimationTracks(checkedItems);
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear##ClearAnimationTracks"))
				{
					animationTracksListView.clear(animationNames);
					siv3dSpinePlayer.addAnimationTracks({});
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

				float duration = siv3dSpinePlayer.getAnimationDuration(fadeOut.c_str());
				if (s3d::NotEqual(duration, 0.f))
				{
					static float mixTime = 0.1f;
					ImGui::SliderFloat("Mix time", &mixTime, 0.f, duration, "%0.2f");

					if (ImGui::Button("Mix##MixAnimations"))
					{
						siv3dSpinePlayer.mixAnimations(fadeOut.c_str(), fadeIn.c_str(), mixTime);
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear##ClearMixedAnimations"))
					{
#if defined (SPINE_4_1_OR_LATER) || defined (SPINE_4_2_OR_LATER)
						siv3dSpinePlayer.clearMixedAnimation();
#else /* Spine 4.0以前 */
						/* Spine::HashMapの操作手段が存在しない。 */
						siv3dSpinePlayer.mixAnimations(fadeOut.c_str(), fadeIn.c_str(), 0.f);
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
			const std::vector<std::string>& skinNames = siv3dSpinePlayer.getSkinNames();
			/* 装い指定 */
			if (ImGui::TreeNode("Set Skin"))
			{
				static ImGuiComboBox skinComboBox;
				skinComboBox.update(skinNames, "##SkinToSet");

				if (ImGui::Button("Apply##SetSkin"))
				{
					siv3dSpinePlayer.setSkinByIndex(skinComboBox.selectedIndex);
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
					siv3dSpinePlayer.mixSkins(checkedItems);
				}
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		} /* 装い */

		/* 槽溝 */
		if (ImGui::BeginTabItem("Slot"))
		{
			const std::vector<std::string>& slotNames = siv3dSpinePlayer.getSlotNames();
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
					siv3dSpinePlayer.setSlotsToExclude(checkedItems);
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##ExcludeSlots"))
				{
					slotListView.clear(slotNames);
					siv3dSpinePlayer.setSlotsToExclude({});
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
				const auto& slotAttachmentMap = siv3dSpinePlayer.getSlotNamesWithTheirAttachments();
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
							siv3dSpinePlayer.replaceAttachment(
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
			bool pma = siv3dSpinePlayer.isAlphaPremultiplied();
#if defined(SPINE_4_0) || defined(SPINE_4_1_OR_LATER) || defined(SPINE_4_2_OR_LATER)
			ImGui::BeginDisabled();
#endif
			if (ImGui::Checkbox("Premultiply alpha", &pma))
			{
				siv3dSpinePlayer.premultiplyAlpha(pma);
			}
			HelpMarker("For Spine 3.8 and older, PMA should be configured manually.\n"
				"For Spine 4.0 and later, PMA property of atlas page is applied.");
#if defined(SPINE_4_0) || defined(SPINE_4_1_OR_LATER) || defined(SPINE_4_2_OR_LATER)
			ImGui::EndDisabled();
#endif

			bool toForceBlendModeNormal = siv3dSpinePlayer.isBlendModeNormalForced();
			if (ImGui::Checkbox("Force blend-mode-normal", &toForceBlendModeNormal))
			{
				siv3dSpinePlayer.forceBlendModeNormal(toForceBlendModeNormal);
			}
			HelpMarker("Force if blend-mode-multiply is not well rendered.");

			bool drawOrder = siv3dSpinePlayer.isDrawOrderReversed();
			bool drawOrderConfigureWorthy = siv3dSpinePlayer.getNumberOfSpines() > 1;
			if (drawOrderConfigureWorthy)
			{
				if (ImGui::Checkbox("Reverse draw order", &drawOrder))
				{
					siv3dSpinePlayer.setDrawOrder(drawOrder);
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

			bool isVisible = siv3dSpinePlayer.isVisible();
			if (ImGui::Checkbox("Visible", &isVisible))
			{
				siv3dSpinePlayer.setVisibility(isVisible);
			}

			bool isPaused = siv3dSpinePlayer.isPaused();
			if (ImGui::Checkbox("Paused", &isPaused))
			{
				siv3dSpinePlayer.setPause(isPaused);
			}

			/* 頂点位置確認用 */
			ImGui::Checkbox("Wireframe", &isWireframeMode);

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
					ScrollableSliderInt("GIF", &imageFps, MinFps, MaxImageFps);
					HelpMarker("GIF delay is defined in 10ms increments.\n Mind that fractional part will be discarded.");
					ScrollableSliderInt("Video", &videoFps, MinFps, MaxVideoFps);
				}
				else
				{
					static int s_imageFps = imageFps;
					static int s_videoFps = videoFps;
					ScrollableSliderInt("GIF", &s_imageFps, MinFps, MaxImageFps);
					ScrollableSliderInt("Video", &s_videoFps, MinFps, MaxVideoFps);
					imageFps = static_cast<s3d::int32>(s_imageFps);
					videoFps = static_cast<s3d::int32>(s_videoFps);
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

void siv3d_imgui_dialogue::ShowHelp()
{
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

	ImGui::Begin("Help", nullptr, ImGuiWindowFlags_NoResize);

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
