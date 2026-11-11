#pragma once
#include "global.h"
#include <mfapi.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Mp4Exporter {
public:
	Mp4Exporter();
	~Mp4Exporter();

	bool init(const std::wstring& path, int width, int height, int fps);
	bool writeFrame(const std::vector<uint8_t>& rgbaPixels);
	void endExporter();

private:
	ComPtr<IMFSinkWriter> m_writer;
	DWORD m_streamIndex = 0;
	int m_width = 0, m_height = 0, m_fps = 0;
	long long m_timestamp = 0;
	bool m_initialized = false;
	std::vector<uint8_t> m_flippedBuffer;
};