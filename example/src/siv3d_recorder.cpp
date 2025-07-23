
#include "siv3d_recorder.h"

#if SIV3D_PLATFORM(WINDOWS)
#include "wic/wic_gif_encoder.h"
#endif


CSiv3dRecorder::CSiv3dRecorder()
{

}

CSiv3dRecorder::~CSiv3dRecorder()
{
	Clear();
}

bool CSiv3dRecorder::Start(const s3d::Size& frameSize, EOutputType outputType, s3d::int32 fps)
{
	Clear();

	m_outputType = outputType;
	m_fps = fps;

	m_frameSize = frameSize;
	m_lastTime = s3d::Scene::Time();

	return true;
}

bool CSiv3dRecorder::IsUnderRecording() const
{
	return m_lastTime.has_value();
}

CSiv3dRecorder::EOutputType CSiv3dRecorder::GetOutputType() const
{
	return m_outputType;
}

bool CSiv3dRecorder::HasTimePassed() const
{
	double currentTime = s3d::Scene::Time();
	double elapsedTime = currentTime - m_lastTime.value();
	double timeToWait = 1.0 / m_fps;

	return ::isgreaterequal(elapsedTime, timeToWait);
}

bool CSiv3dRecorder::CommitFrame(const s3d::RenderTexture& frame)
{
	if (IsUnderRecording())
	{
		if (HasTimePassed())
		{
			s3d::RenderTexture copiedFrame(m_frameSize);
			s3d::Shader::Copy(frame, copiedFrame);
			m_frames.push_back(std::move(copiedFrame));

			m_lastTime = s3d::Scene::Time();

			return true;
		}
	}

	return false;
}

bool CSiv3dRecorder::End(s3d::FilePath& filePath)
{
	if (filePath.empty())
	{
		Clear();
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
		/* Though Siv3D API are designed to create nonexistent directory when to write file, here is outside. */
		s3d::FileSystem::CreateDirectories(s3d::FileSystem::ParentPath(filePath));

		CWicGifEncoder wicGifEncoder;
		wicGifEncoder.Initialise(s3d::Unicode::ToWstring(filePath).c_str());
		if (wicGifEncoder.HasBeenInitialised())
		{
			float delay = static_cast<float>(1.0 / m_fps);
			for (auto& frame : m_frames)
			{
				s3d::Image image;
				frame.readAsImage(image);
				result |= wicGifEncoder.CommitFrame(image.width(), image.height(), image.stride(), image.dataAsUint8(), true, delay);
				frame.release();
			}
			wicGifEncoder.Finalise();
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
	Clear();

	return result;
}

void CSiv3dRecorder::Clear()
{
	m_fps = DefaultFps;
	m_lastTime.reset();

	m_frames.clear();
}
