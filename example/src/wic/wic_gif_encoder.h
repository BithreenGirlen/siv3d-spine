#ifndef WIC_GIF_ENCODER_H_
#define WIC_GIF_ENCODER_H_

class CWicGifEncoder
{
public:
	CWicGifEncoder();
	~CWicGifEncoder();

	bool Initialise(const wchar_t *filePath);
	bool HasBeenInitialised() const;

	/// @brief フレーム書き込み。画素配列はRGBA32を想定。
	bool CommitFrame(unsigned int width, unsigned int height, unsigned int stride, unsigned char* pixels, bool hasAlpha, float delayInSeconds);

	bool Finalise();
private:
	class Impl;
	Impl* m_impl = nullptr;
};

#endif // !WIC_GIF_ENCODER_H_
