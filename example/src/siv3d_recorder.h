#ifndef SIV3d_RECORDER_H_
#define SIV3d_RECORDER_H_

#ifndef NO_S3D_USING
#define NO_S3D_USING
#endif
#include <Siv3D.hpp>

class CSiv3dRecorder
{
public:
	CSiv3dRecorder();
	~CSiv3dRecorder();

	enum class EOutputType
	{
		Gif,
		Video,
	};

	bool start(const s3d::Size &frameSize, EOutputType outputType, s3d::int32 fps = DefaultFps);

	bool isUnderRecording() const;
	EOutputType getOutputType() const;
	s3d::int32 getFps() const;

	/// @brief 前回のフレーム保存時刻から設定時間経過したか
	bool hasTimePassed() const;
	/// @brief フレーム保存。
	bool commitFrame(const s3d::RenderTexture& frame);
	/// @brief 少なくとも1フレーム保存しているか
	bool hasFrames() const;

	/// @param filePath 出力先。拡張子がない場合出力形式に応じて自動付与。
	bool end(s3d::FilePath& filePath);
private:
	static constexpr s3d::int32 DefaultFps = 30;

	s3d::int32 m_fps = DefaultFps;
	s3d::Optional<double> m_lastTime;

	s3d::Array<s3d::RenderTexture> m_frames;
	s3d::Size m_frameSize{};
	EOutputType m_outputType = EOutputType::Video;

	void clear();
};
#endif // !SIV3d_RECORDER_H_
