
#include "siv3d_recorder.h"

#if SIV3D_PLATFORM(WINDOWS)
#include "wic/wic_gif_encoder.h"
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
		const auto &extension = s3d::FileSystem::Extension(filePath);
		if (extension.empty())
		{
			if (m_outputType == EOutputType::Gif)
			{
				filePath += U".gif";
			}
			else if(m_outputType == EOutputType::Video)
			{
				filePath += U".mp4";
			}
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
			float delay = static_cast<float>(1.0 / m_fps);
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
