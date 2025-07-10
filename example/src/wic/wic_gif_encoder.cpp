

#include <atlbase.h>
#include <wincodec.h>

#pragma comment (lib,"Windowscodecs.lib")

#include "wic_gif_encoder.h"

class CWicGifEncoder::Impl
{
public:
	bool Initialise(const wchar_t* filePath);
	bool HasBeenInitialised() const { return m_hasBeenInitialised; }

	bool CommitFrame(unsigned int width, unsigned int height, unsigned int stride, unsigned char* pixels, bool hasAlpha, unsigned short delay);

	bool Finalise();
private:
	CComPtr<IWICImagingFactory> m_pWicImagingFactory;
	CComPtr<IWICBitmapEncoder> m_pWicBitmapEncoder;
	CComPtr<IWICStream> m_pWicStream;

	bool m_hasBeenInitialised = false;
};


bool CWicGifEncoder::Impl::Initialise(const wchar_t* filePath)
{
	HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWicImagingFactory));
	if (FAILED(hr))return false;

	hr = m_pWicImagingFactory->CreateEncoder(GUID_ContainerFormatGif, nullptr, &m_pWicBitmapEncoder);
	if (FAILED(hr))return false;

	hr = m_pWicImagingFactory->CreateStream(&m_pWicStream);
	if (FAILED(hr))return false;

	hr = m_pWicStream->InitializeFromFilename(filePath, GENERIC_WRITE);
	if (FAILED(hr))return false;

	hr = m_pWicBitmapEncoder->Initialize(m_pWicStream, WICBitmapEncoderCacheOption::WICBitmapEncoderNoCache);
	if (FAILED(hr))return false;

	CComPtr<IWICMetadataQueryWriter> pWicMetadataQueryWriter;
	hr = m_pWicBitmapEncoder->GetMetadataQueryWriter(&pWicMetadataQueryWriter);
	if (FAILED(hr))return false;

	const auto SetApplicationMetadata = [&pWicMetadataQueryWriter]()
		-> bool
		{
			PROPVARIANT sPropVariant{};
			sPropVariant.vt = VT_UI1 | VT_VECTOR;
			char szName[] = "NETSCAPE2.0";
			sPropVariant.cac.cElems = sizeof(szName) - 1;
			sPropVariant.cac.pElems = szName;
			HRESULT hr = pWicMetadataQueryWriter->SetMetadataByName(L"/appext/application", &sPropVariant);
			return SUCCEEDED(hr);
		};
	const auto SetDataMetadata = [&pWicMetadataQueryWriter]()
		-> bool
		{
			PROPVARIANT sPropVariant{};
			sPropVariant.vt = VT_UI1 | VT_VECTOR;
			char szLoopCount[] = { 0x03, 0x01, 0x00, 0x00, 0x00 };
			sPropVariant.cac.cElems = sizeof(szLoopCount);
			sPropVariant.cac.pElems = szLoopCount;
			HRESULT hr = pWicMetadataQueryWriter->SetMetadataByName(L"/appext/data", &sPropVariant);
			return SUCCEEDED(hr);
		};
	m_hasBeenInitialised = SetApplicationMetadata() && SetDataMetadata();

	return m_hasBeenInitialised;
}

bool CWicGifEncoder::Impl::CommitFrame(unsigned int width, unsigned int height, unsigned int stride, unsigned char* pixels, bool hasAlpha, unsigned short delay)
{
	CComPtr<IWICBitmapFrameEncode> pWicBitmapFrameEncode;
	CComPtr<IPropertyBag2> pPropertyBag;
	HRESULT hr = m_pWicBitmapEncoder->CreateNewFrame(&pWicBitmapFrameEncode, &pPropertyBag);
	if (FAILED(hr))return false;

	hr = pWicBitmapFrameEncode->Initialize(pPropertyBag);
	if (FAILED(hr))return false;

	hr = pWicBitmapFrameEncode->SetSize(width, height);
	if (FAILED(hr))return false;

	CComPtr<IWICBitmap> pWicBitmap;
	hr = m_pWicImagingFactory->CreateBitmapFromMemory
	(
		width,
		height,
		hasAlpha ? GUID_WICPixelFormat32bppRGBA : GUID_WICPixelFormat32bppRGB,
		stride,
		stride * height,
		pixels,
		&pWicBitmap
	);
	if (FAILED(hr))return false;

	CComPtr<IWICMetadataQueryWriter> pWicMetadataQueryWriter;
	hr = pWicBitmapFrameEncode->GetMetadataQueryWriter(&pWicMetadataQueryWriter);
	if (FAILED(hr))return false;

	const auto SetDelayMetadata = [&pWicMetadataQueryWriter, &delay]()
		-> bool
		{
			PROPVARIANT sPropVariant{};
			sPropVariant.vt = VT_UI2;
			sPropVariant.uiVal = delay;
			HRESULT hr = pWicMetadataQueryWriter->SetMetadataByName(L"/grctlext/Delay", &sPropVariant);
			return SUCCEEDED(hr);
		};

	const auto SetDisposalMetaData = [&pWicMetadataQueryWriter]()
		-> bool
		{
			PROPVARIANT sPropVariant{};
			sPropVariant.vt = VT_UI1;
			sPropVariant.bVal = 2;
			HRESULT hr = pWicMetadataQueryWriter->SetMetadataByName(L"/grctlext/Disposal", &sPropVariant);
			return SUCCEEDED(hr);
		};
	const auto SetTransparencyFlag = [&pWicMetadataQueryWriter]()
		-> bool
		{
			PROPVARIANT sPropVariant{};
			sPropVariant.vt = VT_BOOL;
			sPropVariant.boolVal = 1;
			HRESULT hr = pWicMetadataQueryWriter->SetMetadataByName(L"/grctlext/TransparencyFlag", &sPropVariant);
			return SUCCEEDED(hr);
		};
	const auto SetTransparentColorIndex = [&pWicMetadataQueryWriter]()
		-> bool
		{
			PROPVARIANT sPropVariant{};
			sPropVariant.vt = VT_UI1;
			sPropVariant.bVal = 255;
			HRESULT hr = pWicMetadataQueryWriter->SetMetadataByName(L"/grctlext/TransparentColorIndex", &sPropVariant);
			return SUCCEEDED(hr);
		};

	SetDisposalMetaData() && SetTransparencyFlag() && SetDelayMetadata() && SetTransparentColorIndex();

	hr = pWicBitmapFrameEncode->WriteSource(pWicBitmap, nullptr);
	if (FAILED(hr))return false;

	hr = pWicBitmapFrameEncode->Commit();

	return SUCCEEDED(hr);
}

bool CWicGifEncoder::Impl::Finalise()
{
	HRESULT hr = m_pWicBitmapEncoder->Commit();

	if (SUCCEEDED(hr))
	{
		m_hasBeenInitialised = false;
	}

	return SUCCEEDED(hr);
}



CWicGifEncoder::CWicGifEncoder()
{
	m_impl = new CWicGifEncoder::Impl();
}

CWicGifEncoder::~CWicGifEncoder()
{
	delete m_impl;
}

bool CWicGifEncoder::Initialise(const wchar_t* filePath)
{
	return m_impl->Initialise(filePath);
}

bool CWicGifEncoder::HasBeenInitialised() const
{
	return m_impl->HasBeenInitialised();
}

bool CWicGifEncoder::CommitFrame(unsigned int width, unsigned int height, unsigned int stride, unsigned char* pixels, bool hasAlpha, float delayInSeconds)
{
	/* 10ms単位 */
	unsigned short delayInHundredths = static_cast<unsigned short>(delayInSeconds * 100.f);

	return m_impl->CommitFrame(width, height, stride, pixels, hasAlpha, delayInHundredths == 0 ? 1 : delayInHundredths);
}

bool CWicGifEncoder::Finalise()
{
	return m_impl->Finalise();
}
