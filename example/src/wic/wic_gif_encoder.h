#ifndef WIC_GIF_ENCODER_H_
#define WIC_GIF_ENCODER_H_

class CWicGifEncoder
{
public:
	CWicGifEncoder();
	~CWicGifEncoder();

	bool initialise(const wchar_t* filePath);
	bool hasBeenInitialised() const;

	/// @brief フレーム書き込み。画素配列はRGBA32を想定。
	bool commitFrame(unsigned int width, unsigned int height, unsigned int stride, unsigned char* pixels, bool hasAlpha, float delayInSeconds);

	bool finalise();
private:
	class Impl;
	Impl* m_impl = nullptr;
};

#endif // !WIC_GIF_ENCODER_H_
