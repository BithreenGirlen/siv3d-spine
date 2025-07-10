
#include "siv3d_imgui.h"

/*
* https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md#writing-your-own-backend
*/

/// @brief To be stored as io.BackendPlatformUserData.
struct UserDatum
{
	/// @brief font data to be held when added via memory.
	s3d::Array<s3d::Array<s3d::Byte>> fontDataList;

	/// @brief vertices and indices for rendering
	s3d::Buffer2D buffer2d;
};

/* Internal functions; not corfomance to the nomenclature of Dear-Imgui. */
namespace siv3d_imgui
{
	static void UpdateMouseEvent()
	{
		ImGuiIO& io = ImGui::GetIO();

		/* Should be client coördinate. */
		const auto& clientMousePos = s3d::Cursor::Pos();
		io.AddMousePosEvent(static_cast<float>(clientMousePos.x), static_cast<float>(clientMousePos.y));

		for (const s3d::Input& mouseInput : s3d::Mouse::GetAllInputs())
		{
			if (mouseInput.code() >= ImGuiMouseButton_COUNT)break;

			/* Cannot distinguish the input coming from touch-screen from that from mouse. */

			io.AddMouseButtonEvent(mouseInput.code(), mouseInput.pressed());
		}

		const s3d::Vector2D<float> wheelScroll = { static_cast<float>(s3d::Mouse::WheelH()), static_cast<float>(s3d::Mouse::Wheel()) };
		io.AddMouseWheelEvent(-wheelScroll.x, -wheelScroll.y);
	}

	static s3d::CursorStyle GetMouseCursorStyle()
	{
		/* No counterpart to ImGuiMouseCursor_Wait and ImGuiMouseCursor_Progress. */
		switch (ImGui::GetMouseCursor())
		{
		case ImGuiMouseCursor_None: return s3d::CursorStyle::Hidden;
		case ImGuiMouseCursor_Arrow: return s3d::CursorStyle::Arrow;
		case ImGuiMouseCursor_TextInput: return s3d::CursorStyle::IBeam;
		case ImGuiMouseCursor_ResizeAll: return s3d::CursorStyle::ResizeAll;
		case ImGuiMouseCursor_ResizeEW: return s3d::CursorStyle::ResizeLeftRight;
		case ImGuiMouseCursor_ResizeNS: return s3d::CursorStyle::ResizeUpDown;
		case ImGuiMouseCursor_ResizeNESW: return s3d::CursorStyle::ResizeNESW;
		case ImGuiMouseCursor_ResizeNWSE: return s3d::CursorStyle::ResizeNWSE;
		case ImGuiMouseCursor_Hand: return s3d::CursorStyle::Hand;
		case ImGuiMouseCursor_NotAllowed: return s3d::CursorStyle::NotAllowed;
		default: break;
		}

		return s3d::CursorStyle::Default;
	}

	static void UpdateMouseCursor()
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)return;

		/* Should be called every frame, without comparing with the last state. */
		if (io.MouseDrawCursor)
		{
			s3d::Cursor::RequestStyle(s3d::CursorStyle::Hidden);
		}
		else
		{
			s3d::CursorStyle cursorStyle = GetMouseCursorStyle();
			s3d::Cursor::RequestStyle(cursorStyle);
		}
	}

	static void UpdateKeyModifiers()
	{
		ImGuiIO& io = ImGui::GetIO();

		io.AddKeyEvent(ImGuiMod_Ctrl, s3d::KeyControl.pressed());
		io.AddKeyEvent(ImGuiMod_Shift, s3d::KeyShift.pressed());
		io.AddKeyEvent(ImGuiMod_Alt, s3d::KeyAlt.pressed());
	}

	static constexpr ImGuiKey Siv3dInputToImGuiInput(s3d::uint8 keyCode)
	{
		switch (keyCode)
		{
		case s3d::KeyTab.code(): return ImGuiKey_Tab;
		case s3d::KeyLeft.code(): return ImGuiKey_LeftArrow;
		case s3d::KeyRight.code(): return ImGuiKey_RightArrow;
		case s3d::KeyUp.code(): return ImGuiKey_UpArrow;
		case s3d::KeyDown.code(): return ImGuiKey_DownArrow;
		case s3d::KeyPageUp.code(): return ImGuiKey_PageUp;
		case s3d::KeyPageDown.code(): return ImGuiKey_PageDown;
		case s3d::KeyHome.code(): return ImGuiKey_Home;
		case s3d::KeyEnd.code(): return ImGuiKey_End;
		case s3d::KeyInsert.code(): return ImGuiKey_Insert;
		case s3d::KeyDelete.code(): return ImGuiKey_Delete;
		case s3d::KeyBackspace.code(): return ImGuiKey_Backspace;
		case s3d::KeySpace.code(): return ImGuiKey_Space;
		case s3d::KeyEnter.code(): return ImGuiKey_Enter;
		case s3d::KeyEscape.code(): return ImGuiKey_Escape;
		case s3d::KeyLControl.code(): return ImGuiKey_LeftCtrl;
		case s3d::KeyLShift.code(): return ImGuiKey_LeftShift;
		case s3d::KeyLAlt.code(): return ImGuiKey_LeftAlt;
		case s3d::KeyRControl.code(): return ImGuiKey_RightCtrl;
		case s3d::KeyRShift.code(): return ImGuiKey_RightShift;
		case s3d::KeyRAlt.code(): return ImGuiKey_RightAlt;
		case s3d::Key0.code(): return ImGuiKey_0;
		case s3d::Key1.code(): return ImGuiKey_1;
		case s3d::Key2.code(): return ImGuiKey_2;
		case s3d::Key3.code(): return ImGuiKey_3;
		case s3d::Key4.code(): return ImGuiKey_4;
		case s3d::Key5.code(): return ImGuiKey_5;
		case s3d::Key6.code(): return ImGuiKey_6;
		case s3d::Key7.code(): return ImGuiKey_7;
		case s3d::Key8.code(): return ImGuiKey_8;
		case s3d::Key9.code(): return ImGuiKey_9;
		case s3d::KeyA.code(): return ImGuiKey_A;
		case s3d::KeyB.code(): return ImGuiKey_B;
		case s3d::KeyC.code(): return ImGuiKey_C;
		case s3d::KeyD.code(): return ImGuiKey_D;
		case s3d::KeyE.code(): return ImGuiKey_E;
		case s3d::KeyF.code(): return ImGuiKey_F;
		case s3d::KeyG.code(): return ImGuiKey_G;
		case s3d::KeyH.code(): return ImGuiKey_H;
		case s3d::KeyI.code(): return ImGuiKey_I;
		case s3d::KeyJ.code(): return ImGuiKey_J;
		case s3d::KeyK.code(): return ImGuiKey_K;
		case s3d::KeyL.code(): return ImGuiKey_L;
		case s3d::KeyM.code(): return ImGuiKey_M;
		case s3d::KeyN.code(): return ImGuiKey_N;
		case s3d::KeyO.code(): return ImGuiKey_O;
		case s3d::KeyP.code(): return ImGuiKey_P;
		case s3d::KeyQ.code(): return ImGuiKey_Q;
		case s3d::KeyR.code(): return ImGuiKey_R;
		case s3d::KeyS.code(): return ImGuiKey_S;
		case s3d::KeyT.code(): return ImGuiKey_T;
		case s3d::KeyU.code(): return ImGuiKey_U;
		case s3d::KeyV.code(): return ImGuiKey_V;
		case s3d::KeyW.code(): return ImGuiKey_W;
		case s3d::KeyX.code(): return ImGuiKey_X;
		case s3d::KeyY.code(): return ImGuiKey_Y;
		case s3d::KeyZ.code(): return ImGuiKey_Z;
		case s3d::KeyF1.code(): return ImGuiKey_F1;
		case s3d::KeyF2.code(): return ImGuiKey_F2;
		case s3d::KeyF3.code(): return ImGuiKey_F3;
		case s3d::KeyF4.code(): return ImGuiKey_F4;
		case s3d::KeyF5.code(): return ImGuiKey_F5;
		case s3d::KeyF6.code(): return ImGuiKey_F6;
		case s3d::KeyF7.code(): return ImGuiKey_F7;
		case s3d::KeyF8.code(): return ImGuiKey_F8;
		case s3d::KeyF9.code(): return ImGuiKey_F9;
		case s3d::KeyF10.code(): return ImGuiKey_F10;
		case s3d::KeyF11.code(): return ImGuiKey_F11;
		case s3d::KeyF12.code(): return ImGuiKey_F12;
		case s3d::KeyApostrophe_US.code(): return ImGuiKey_Apostrophe;
		case s3d::KeyComma.code(): return ImGuiKey_Comma;
		case s3d::KeyMinus.code(): return ImGuiKey_Minus;
		case s3d::KeyPeriod.code(): return ImGuiKey_Period;
		case s3d::KeySlash.code(): return ImGuiKey_Slash;
		case s3d::KeySemicolon_US.code(): return ImGuiKey_Semicolon;
		case s3d::KeyEqual_US.code(): return ImGuiKey_Equal;
		case s3d::KeyLBracket.code(): return ImGuiKey_LeftBracket;
		case s3d::KeyBackslash_US.code(): return ImGuiKey_Backslash;
		case s3d::KeyRBracket.code(): return ImGuiKey_RightBracket;
		case s3d::KeyGraveAccent.code(): return ImGuiKey_GraveAccent;
		case s3d::KeyNumLock.code(): return ImGuiKey_NumLock;
		case s3d::KeyPrintScreen.code(): return ImGuiKey_PrintScreen;
		case s3d::KeyPause.code(): return ImGuiKey_Pause;
		case s3d::KeyNum0.code(): return ImGuiKey_Keypad0;
		case s3d::KeyNum1.code(): return ImGuiKey_Keypad1;
		case s3d::KeyNum2.code(): return ImGuiKey_Keypad2;
		case s3d::KeyNum3.code(): return ImGuiKey_Keypad3;
		case s3d::KeyNum4.code(): return ImGuiKey_Keypad4;
		case s3d::KeyNum5.code(): return ImGuiKey_Keypad5;
		case s3d::KeyNum6.code(): return ImGuiKey_Keypad6;
		case s3d::KeyNum7.code(): return ImGuiKey_Keypad7;
		case s3d::KeyNum8.code(): return ImGuiKey_Keypad8;
		case s3d::KeyNum9.code(): return ImGuiKey_Keypad9;
		case s3d::KeyNumDecimal.code(): return ImGuiKey_KeypadDecimal;
		case s3d::KeyNumDivide.code(): return ImGuiKey_KeypadDivide;
		case s3d::KeyNumMultiply.code(): return ImGuiKey_KeypadMultiply;
		case s3d::KeyNumSubtract.code(): return ImGuiKey_KeypadSubtract;
		case s3d::KeyNumAdd.code(): return ImGuiKey_KeypadAdd;
		case s3d::KeyNumEnter.code(): return ImGuiKey_KeypadEnter;
		case s3d::KeyControl.code(): return ImGuiKey_ModCtrl;
		case s3d::KeyShift.code(): return ImGuiKey_ModShift;
		case s3d::KeyAlt.code(): return ImGuiKey_ModAlt;
		default: break;
		}

		return ImGuiKey_None;
	}

	static void UpdateKeyboardEvent()
	{
		ImGuiIO& io = ImGui::GetIO();

		for (const auto& keyboardInput : s3d::Keyboard::GetAllInputs())
		{
			const ImGuiKey imguiKey = Siv3dInputToImGuiInput(keyboardInput.code());

			io.AddKeyEvent(imguiKey, keyboardInput.pressed());
		}

		s3d::String rawInput = s3d::TextInput::GetRawInput();
		if (!rawInput.empty())
		{
			io.AddInputCharactersUTF8(s3d::Unicode::ToUTF8(rawInput).c_str());
		}
	}

	static void DestroyTexture(ImTextureData* pTextureData)
	{
		s3d::RenderTexture* pRenderTexture = reinterpret_cast<s3d::RenderTexture*>(pTextureData->GetTexID());
		if (pRenderTexture != nullptr)
		{
			delete pRenderTexture;

			pTextureData->SetTexID(ImTextureID_Invalid);
			pTextureData->SetStatus(ImTextureStatus_Destroyed);
		}
	}

	struct BlendMode
	{
		/// @brief blend-mode to be used to update region of font altas
		static constexpr s3d::BlendState Normal = s3d::BlendState
		{
			true,
			s3d::Blend::SrcAlpha, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add,
			s3d::Blend::One, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add
		};
	};

	static void UploadTexture(ImTextureData* pTextureData)
	{
		if (pTextureData->Status == ImTextureStatus_WantCreate)
		{
			/* Accepts only R8G8B8A8 format */
			if (pTextureData->Format != ImTextureFormat_RGBA32)return;

			s3d::Image image(pTextureData->Width, pTextureData->Height);
			memcpy(image.dataAsUint8(), pTextureData->Pixels, pTextureData->GetSizeInBytes());
			s3d::RenderTexture* pRenderTexture = new s3d::RenderTexture(image);

			/* ImTextureID is 64bits, so need not to be widen. */
			pTextureData->SetTexID(reinterpret_cast<ImTextureID>(pRenderTexture));
			pTextureData->SetStatus(ImTextureStatus_OK);
		}
		else if (pTextureData->Status == ImTextureStatus_WantUpdates)
		{
			/* Pretend UpdateSubresource() or glTexSubImage2D() */
			s3d::RenderTexture* pRenderTexture = reinterpret_cast<s3d::RenderTexture*>(pTextureData->GetTexID());
			if (pRenderTexture != nullptr)
			{
				const s3d::ScopedRenderStates2D scopedRenderState(BlendMode::Normal, s3d::SamplerState::ClampLinear);
				const s3d::ScopedRenderTarget2D scopedRenderTarget(*pRenderTexture);

				for (const ImTextureRect& r : pTextureData->Updates)
				{
					s3d::Image image(r.w, r.h);
					unsigned int stride = r.w * pTextureData->BytesPerPixel;
					for (unsigned short y = 0; y < r.h; ++y)
					{
						memcpy(image.dataAsUint8() + stride * y, pTextureData->GetPixelsAt(r.x, r.y + y), stride);
					}

					const s3d::Texture texture(image);
					texture.draw(r.x, r.y);
					s3d::Graphics2D::Flush();
				}
			}
			pTextureData->SetStatus(ImTextureStatus_OK);
		}

		if (pTextureData->Status == ImTextureStatus_WantDestroy && pTextureData->UnusedFrames > 0)
		{
			DestroyTexture(pTextureData);
		}
	}

	/// @brief Optional 
	namespace callbacks
	{
		static const char* GetClipboardText(ImGuiContext* ctx)
		{
			IM_UNUSED(ctx);

			s3d::String sBuffer;
			s3d::Clipboard::GetText(sBuffer);
			return sBuffer.empty() ? nullptr : s3d::Unicode::ToUTF8(sBuffer).c_str();
		}

		static void SetClipboardText(ImGuiContext* ctx, const char* text)
		{
			IM_UNUSED(ctx);

			if (text != nullptr)
			{
				s3d::Clipboard::SetText(s3d::Unicode::FromUTF8(text));
			}
		}
	}
}


bool ImGui_ImplSiv3d_Init()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.BackendPlatformUserData != nullptr)return false;

	auto* pBackEndData = IM_NEW(UserDatum)();

	io.BackendPlatformName = "imgui_impl_siv3d";
	io.BackendPlatformUserData = pBackEndData;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures; // For UploadTexture to be called.

	ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
	platformIo.Platform_GetClipboardTextFn = &siv3d_imgui::callbacks::GetClipboardText;
	platformIo.Platform_SetClipboardTextFn = &siv3d_imgui::callbacks::SetClipboardText;

	/* No shell, IME option. */

	return true;
}

void ImGui_ImplSiv3d_NewFrame()
{
	ImGuiIO& io = ImGui::GetIO();

	io.DeltaTime = static_cast<float>(s3d::Scene::DeltaTime());
	{
		const auto& windowState = s3d::Window::GetState();

		const auto& windowSize = windowState.frameBufferSize;
		const auto& scaling = static_cast<float>(windowState.scaling);

		io.DisplaySize = { static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) };
		io.DisplayFramebufferScale = { scaling, scaling };
	}

	siv3d_imgui::UpdateMouseCursor();
	siv3d_imgui::UpdateKeyModifiers();
	siv3d_imgui::UpdateMouseEvent();
	siv3d_imgui::UpdateKeyboardEvent();

	/* No gamepad implementation. */
}

void ImGui_ImplSiv3d_RenderDrawData()
{
	ImDrawData* pDrawData = ImGui::GetDrawData();
	if (pDrawData->Textures != nullptr)
	{
		for (ImTextureData* textureData : *pDrawData->Textures)
		{
			if (textureData->Status != ImTextureStatus_OK)
			{
				siv3d_imgui::UploadTexture(textureData);
			}
		}
	}

	UserDatum* p = static_cast<UserDatum*>(ImGui::GetIO().BackendPlatformUserData);
	if (p == nullptr)return;
	auto& buffer2d = p->buffer2d;

	s3d::Rect foreScissorRect = s3d::Graphics2D::GetScissorRect();
	const s3d::ScopedRenderStates2D scopedRenderState(s3d::RasterizerState::SolidCullNoneScissor);

	ImVec2 clipOffset = pDrawData->DisplayPos;
	/* Scale should not be applied to clipping rect. */
	for (const auto* pCommandList : pDrawData->CmdLists)
	{
		size_t indexOffset = 0;

		for (const auto& drawCommand : pCommandList->CmdBuffer)
		{
			if (drawCommand.UserCallback)
			{
				if (drawCommand.UserCallback == ImDrawCallback_ResetRenderState)
				{
					s3d::Graphics2D::SetScissorRect(foreScissorRect);
				}
				else
				{
					drawCommand.UserCallback(pCommandList, &drawCommand);
				}
			}
			else
			{
				buffer2d.vertices.resize(pCommandList->VtxBuffer.Size);
				for (int i = 0; i < pCommandList->VtxBuffer.Size; ++i)
				{
					const ImDrawVert& imguiVertex = pCommandList->VtxBuffer[i];
					s3d::Vertex2D& s3dVertex = buffer2d.vertices[i];

					s3dVertex.pos.x = imguiVertex.pos.x;
					s3dVertex.pos.y = imguiVertex.pos.y;

					s3dVertex.tex.x = imguiVertex.uv.x;
					s3dVertex.tex.y = imguiVertex.uv.y;

					s3dVertex.color.x = (imguiVertex.col & 0xff) / 255.f;
					s3dVertex.color.y = ((imguiVertex.col >> 8) & 0xff) / 255.f;
					s3dVertex.color.z = ((imguiVertex.col >> 16) & 0xff) / 255.f;
					s3dVertex.color.w = ((imguiVertex.col >> 24) & 0xff) / 255.f;
				}

				buffer2d.indices.resize(pCommandList->IdxBuffer.Size / 3);
				memcpy(buffer2d.indices.data(), pCommandList->IdxBuffer.Data, pCommandList->IdxBuffer.size_in_bytes());

				const s3d::Rect clipRect
				(
					static_cast<s3d::int32>(drawCommand.ClipRect.x - clipOffset.x),
					static_cast<s3d::int32>(drawCommand.ClipRect.y - clipOffset.y),
					static_cast<s3d::int32>(drawCommand.ClipRect.z - drawCommand.ClipRect.x),
					static_cast<s3d::int32>(drawCommand.ClipRect.w - drawCommand.ClipRect.y)
				);
				s3d::Graphics2D::SetScissorRect(clipRect);

				s3d::RenderTexture* pRenderTexture = reinterpret_cast<s3d::RenderTexture*>(drawCommand.GetTexID());
				const s3d::uint32 triangleCount = drawCommand.ElemCount / 3;
				buffer2d.drawSubset(indexOffset, triangleCount, *pRenderTexture);

				indexOffset += triangleCount;
			}
		}
	}

	s3d::Graphics2D::SetScissorRect(foreScissorRect);
}

void ImGui_ImplSiv3d_Shutdown()
{
	for (ImTextureData* pTextureData : ImGui::GetPlatformIO().Textures)
	{
		if (pTextureData->RefCount == 1)
		{
			siv3d_imgui::DestroyTexture(pTextureData);
		}
	}

	const ImGuiIO& io = ImGui::GetIO();
	UserDatum* p = static_cast<UserDatum*>(io.BackendPlatformUserData);
	if (p != nullptr)
	{
		IM_DELETE(p);
	}
}

void ImGui_ImplSiv3d_HoldFontData(s3d::Array<s3d::Byte>& fontData)
{
	const ImGuiIO& io = ImGui::GetIO();
	UserDatum* p = static_cast<UserDatum*>(io.BackendPlatformUserData);
	if (p != nullptr)
	{
		p->fontDataList.push_back(std::move(fontData));
	}
}
