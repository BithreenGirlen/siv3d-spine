#ifndef SIV3D_SPINE_BLENDMODE_H_
#define SIV3D_SPINE_BLENDMODE_H_

#include <Siv3D/BlendState.hpp>

#ifdef _MSC_VER
struct Siv3dSpineBlendMode abstract final
#else
struct Siv3dSpineBlendMode
#endif
{
	/// @brief 混色法: 通常
	static constexpr s3d::BlendState Normal = s3d::BlendState
	{
		true,
		s3d::Blend::SrcAlpha, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add,
		s3d::Blend::One, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add
	};
	/// @brief 混色法: 乗算済み通常
	static constexpr s3d::BlendState NormalPma = s3d::BlendState
	{
		true,
		s3d::Blend::One, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add,
		s3d::Blend::One, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add
	};
	/// @brief 混色法: 加算
	static constexpr s3d::BlendState Add = s3d::BlendState
	{
		true,
		s3d::Blend::SrcAlpha, s3d::Blend::One, s3d::BlendOp::Add,
		s3d::Blend::One, s3d::Blend::One, s3d::BlendOp::Add
	};
	/// @brief 混色法: 乗算済み加算
	static constexpr s3d::BlendState AddPma = s3d::BlendState
	{
		true,
		s3d::Blend::One, s3d::Blend::One, s3d::BlendOp::Add,
		s3d::Blend::One, s3d::Blend::One, s3d::BlendOp::Add
	};
	/// @brief 混色法: 乗算
	static constexpr s3d::BlendState Multiply = s3d::BlendState
	{
		true,
		s3d::Blend::DestColor, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add,
		s3d::Blend::One, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add
	};
	/// @brief 混色法: 反転乗算
	static constexpr s3d::BlendState Screen = s3d::BlendState
	{
		true,
		s3d::Blend::One, s3d::Blend::InvSrcColor, s3d::BlendOp::Add,
		s3d::Blend::One, s3d::Blend::InvSrcAlpha, s3d::BlendOp::Add
	};
};

#endif // !SIV3D_SPINE_BLENDMODE_H_
