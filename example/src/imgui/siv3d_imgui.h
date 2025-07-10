#ifndef SIV3D_IMGUI_H_
#define SIV3D_IMGUI_H_

#define NO_S3D_USING
#include <Siv3D.hpp>

#include <imgui.h>

/* Extern functions comfort to the nomenclature of Dear-Imgui. */

bool ImGui_ImplSiv3d_Init();
void ImGui_ImplSiv3d_NewFrame();
void ImGui_ImplSiv3d_RenderDrawData();
void ImGui_ImplSiv3d_Shutdown();

/// @brief Font data held as long as fontAtlas is alive.
/// @param fontData Binary font data
void ImGui_ImplSiv3d_HoldFontData(s3d::Array<s3d::Byte>& fontData);

#endif // !SIV3D_IMGUI_H_
