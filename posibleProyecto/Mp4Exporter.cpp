#include "Mp4Exporter.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

Mp4Exporter::Mp4Exporter() {}

Mp4Exporter::~Mp4Exporter() {
	if (m_initialized) {
		endExporter();
	}
}

bool Mp4Exporter::init(const std::wstring& path, int width, int height, int fps) {
	m_width = width;
	m_height = height;
	m_fps = fps;
	m_flippedBuffer.resize(width * height * 4);

	HRESULT hr = MFStartup(MF_VERSION);
	if (FAILED(hr)) {
		return false;
	}

	hr = MFCreateSinkWriterFromURL(path.c_str(), nullptr, nullptr, &m_writer);
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IMFMediaType> outType;
	MFCreateMediaType(&outType);
	outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	outType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	outType->SetUINT32(MF_MT_AVG_BITRATE, 8000000);
	outType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(outType.Get(), MF_MT_FRAME_SIZE, width, height);
    MFSetAttributeRatio(outType.Get(), MF_MT_FRAME_RATE, fps, 1);
    MFSetAttributeRatio(outType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

	hr = m_writer->AddStream(outType.Get(), &m_streamIndex);
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IMFMediaType> inType;
	ComPtr<IMFMediaType> inType;
	MFCreateMediaType(&inType);
	inType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	inType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	inType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	MFSetAttributeSize(inType.Get(), MF_MT_FRAME_SIZE, width, height);
	MFSetAttributeRatio(inType.Get(), MF_MT_FRAME_RATE, fps, 1);
	MFSetAttributeRatio(inType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

	hr = m_writer->SetInputMediaType(m_streamIndex, inType.Get(), nullptr);
	if (FAILED(hr)) {
		return false;
	}

	hr = m_writer->BeginWriting();
	if (FAILED(hr)) {
		return false;
	}

	m_initialized = true;
	m_timestamp = 0;
	return true;
}

bool Mp4Exporter::writeFrame(const std::vector<uint8_t>& rgbaPixels) {
	if (!m_initialized) {
		return false;
	}

	const int rowSize = m_width * 4;
	for (int y = 0; y < m_height; y++)
	{
		const uint8_t* srcRow = rgbaPixels.data() + (m_height - 1 - y) * rowSize;
		uint8_t* dstRow = m_flippedBuffer.data() + y * rowSize;
		memcpy(dstRow, srcRow, rowSize);
	}

	ComPtr<IMFMediaBuffer> buffer;
	const DWORD bufferSize = m_width * m_height * 4;
	HRESULT hr = MFCreateMemoryBuffer(bufferSize, &buffer);
	if (FAILED(hr)) {
		return false;
	}

	BYTE* pData = nullptr;
	hr = buffer->Lock(&pData, nullptr, nullptr);
	if (FAILED(hr)) {
		return false;
	}
	memcpy(pData, m_flippedBuffer.data(), bufferSize);
	buffer->Unlock();
	buffer->SetCurrentLength(bufferSize);

	ComPtr<IMFSample> sample;
	MFCreateSample(&sample);
	sample->AddBuffer(buffer.Get());
	sample->SetSampleTime(m_timestamp);
	sample->SetSampleDuration(10000000LL / m_fps);

	hr = m_writer->WriteSample(m_streamIndex, sample.Get());
	m_timestamp += 10000000LL / m_fps;
	return SUCCEEDED(hr);
}

void Mp4Exporter::endExporter() {
	if (!m_initialized) {
		return;
	}

	m_writer->Finalize();
	m_writer.Reset();
	MFShutdown();
	m_initialized = false;
}