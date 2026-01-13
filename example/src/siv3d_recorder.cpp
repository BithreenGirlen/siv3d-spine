
#include "siv3d_recorder.h"

#if SIV3D_PLATFORM(WINDOWS)
	#include "windows/wic_gif_encoder.h"
	#include "windows/mf_video_encoder.h"
#endif


CSiv3dRecorder::CSiv3dRecorder()
{

}

CSiv3dRecorder::~CSiv3dRecorder()
{
	clear();
}

bool CSiv3dRecorder::start(const s3d::Size& frameSize, EOutputType outputType, s3d::int32 fps)
{
	clear();

	m_outputType = outputType;
	m_fps = fps;

	m_frameSize = frameSize;
	m_lastTime = s3d::Scene::Time();

	return true;
}

bool CSiv3dRecorder::isUnderRecording() const
{
	return m_lastTime.has_value();
}

CSiv3dRecorder::EOutputType CSiv3dRecorder::getOutputType() const
{
	return m_outputType;
}

s3d::int32 CSiv3dRecorder::getFps() const
{
	return m_fps;
}

bool CSiv3dRecorder::hasTimePassed() const
{
	double currentTime = s3d::Scene::Time();
	double elapsedTime = currentTime - m_lastTime.value();
	double timeToWait = 1.0 / m_fps;

	return s3d::GreaterThanEqual(elapsedTime, timeToWait);
}

bool CSiv3dRecorder::commitFrame(const s3d::RenderTexture& frame)
{
	if (isUnderRecording())
	{
		s3d::RenderTexture copiedFrame(m_frameSize);
		s3d::Shader::Copy(frame, copiedFrame);
		m_frames.push_back(std::move(copiedFrame));

		m_lastTime = s3d::Scene::Time();

		return true;
	}

	return false;
}

bool CSiv3dRecorder::hasFrames() const
{
	return !m_frames.empty();
}

bool CSiv3dRecorder::end(s3d::FilePath& filePath)
{
	if (filePath.empty())
	{
		clear();
		return false;
	}
	else
	{
		if (const auto& extension = s3d::FileSystem::Extension(filePath); extension.empty())
		{
			if (m_outputType == EOutputType::Gif)
			{
				filePath += U".gif";
			}
			else if (m_outputType == EOutputType::Video)
			{
				filePath += U".mp4";
			}
		}

		if (const auto& parentPath = s3d::FileSystem::ParentPath(filePath); not s3d::FileSystem::Exists(parentPath))
		{
			s3d::FileSystem::CreateDirectories(parentPath);
		}
	}

	bool result = false;
	if (m_outputType == EOutputType::Gif)
	{
#if SIV3D_PLATFORM(WINDOWS)
		CWicGifEncoder wicGifEncoder;
		wicGifEncoder.initialise(s3d::Unicode::ToWstring(filePath).c_str());
		if (wicGifEncoder.hasBeenInitialised())
		{
			const float delay = static_cast<float>(1.0 / m_fps);
			for (auto& frame : m_frames)
			{
				s3d::Image image;
				frame.readAsImage(image);
				result |= wicGifEncoder.commitFrame(image.width(), image.height(), image.stride(), image.dataAsUint8(), true, delay);
				frame.release();
			}
			wicGifEncoder.finalise();
		}
#else
		s3d::AnimatedGIFWriter animatedGifWriter;
		animatedGifWriter.open(filePath, m_frameSize.x, m_frameSize.y, s3d::YesNo<s3d::Dither_tag>::Yes, s3d::YesNo<s3d::HasAlpha_tag>::Yes);
		if (animatedGifWriter.isOpen())
		{
			s3d::Duration delay = s3d::SecondsF(1.0 / m_fps);
			for (auto& frame : m_frames)
			{
				s3d::Image image;
				frame.readAsImage(image);
				for (auto& pixel : image)
				{
					/* そのままの透過度では輪郭が崩れるので補正する。 */
					if (pixel.a > 127)
					{
						pixel.a = 255;
					}
				}

				result |= animatedGifWriter.writeFrame(image, delay);
				frame.release();
			}
			animatedGifWriter.close();
		}
#endif
	}
	else if (m_outputType == EOutputType::Video)
	{
#if SIV3D_PLATFORM(WINDOWS)
		CMfVideoEncoder mfVideoEncorder;
		/* 省電力系AMD製CPUでは幅・高さは4の倍数長でないと書き出しDLL内でハングする。 */
		bool toBeTruncated = s3d::GetCPUInfo().vendor.contains(U"AMD");
		if (toBeTruncated)
		{
			m_frameSize.x &= 0xfffffffc;
			m_frameSize.y &= 0xfffffffc;
		}

		mfVideoEncorder.initialise(s3d::Unicode::ToWstring(filePath).c_str(), m_frameSize.x, m_frameSize.y, m_fps);
		if (mfVideoEncorder.hasBeenInitialised())
		{
			for (auto& frame : m_frames)
			{
				s3d::Image image;
				frame.readAsImage(image);
				if (toBeTruncated)
				{
					image = image.clipped({}, { image.width() & 0xfffffffc, image.height() & 0xfffffffc });
				}

				const s3d::uint32 pixelSize = static_cast<s3d::uint32>(image.stride() * image.height());
				result |= mfVideoEncorder.commitCpuFrame(image.dataAsUint8(), pixelSize);
				frame.release();
			}
			mfVideoEncorder.finalise();
		}
#else
		s3d::VideoWriter videoWriter;
		videoWriter.open(filePath, m_frameSize, m_fps);
		if (videoWriter.isOpen())
		{
			for (auto& frame : m_frames)
			{
				s3d::Image image;
				frame.readAsImage(image);

				result |= videoWriter.writeFrame(image);
				frame.release();
			}
			videoWriter.close();
		}
#endif
	}
	clear();

	return result;
}

void CSiv3dRecorder::clear()
{
	m_fps = DefaultFps;
	m_lastTime.reset();

	m_frames.clear();
}
