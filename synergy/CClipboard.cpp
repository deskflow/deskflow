#include "CClipboard.h"

//
// CClipboard
//

CClipboard::CClipboard()
{
	open(0);
	close();
}

CClipboard::~CClipboard()
{
	// do nothing
}

bool					CClipboard::open(Time time)
{
	// clear all data
	for (SInt32 index = 0; index < kNumFormats; ++index) {
		m_data[index]  = "";
		m_added[index] = false;
	}

	// save time
	m_time = time;

	return true;
}

void					CClipboard::close()
{
	// do nothing
}

void					CClipboard::add(EFormat format, const CString& data)
{
	m_data[format]  = data;
	m_added[format] = true;
}

CClipboard::Time		CClipboard::getTime() const
{
	return m_time;
}

bool					CClipboard::has(EFormat format) const
{
	return m_added[format];
}

CString					CClipboard::get(EFormat format) const
{
	return m_data[format];
}

void					CClipboard::copy(IClipboard* dst, const IClipboard* src)
{
	assert(dst != NULL);
	assert(src != NULL);

	copy(dst, src, src->getTime());
}

void					CClipboard::copy(IClipboard* dst,
								const IClipboard* src, Time time)
{
	assert(dst != NULL);
	assert(src != NULL);

	if (dst->open(time)) {
		for (SInt32 format = 0; format != IClipboard::kNumFormats; ++format) {
			IClipboard::EFormat eFormat = (IClipboard::EFormat)format;
			if (src->has(eFormat)) {
				dst->add(eFormat, src->get(eFormat));
			}
		}
		dst->close();
	}
}

void					CClipboard::unmarshall(const CString& data, Time time)
{
	const char* index = data.data();

	// clear existing data
	open(time);

	// read the number of formats
	const UInt32 numFormats = readUInt32(index);
	index += 4;

	// read each format
	for (UInt32 format = 0; format < numFormats; ++format) {
		// get the format id
		UInt32 format = readUInt32(index);
		index += 4;

		// get the size of the format data
		UInt32 size = readUInt32(index);
		index += 4;

		// save the data
		m_added[format] = true;
		m_data[format]  = CString(index, size);
		index += size;
	}

	// done
	close();
}

CString					CClipboard::marshall() const
{
	CString data;

	// compute size of marshalled data
	UInt32 size = 4;
	UInt32 numFormats = 0;
	UInt32 format;
	for (format = 0; format != IClipboard::kNumFormats; ++format) {
		if (m_added[format]) {
			++numFormats;
			size += 4 + 4 + m_data[format].size();
		}
	}

	// allocate space
	data.reserve(size);

	// marshall the data
	writeUInt32(&data, numFormats);
	for (format = 0; format != IClipboard::kNumFormats; ++format) {
		if (m_added[format]) {
			writeUInt32(&data, format);
			writeUInt32(&data, m_data[format].size());
			data += m_data[format];
		}
	}

	return data;
}

UInt32					CClipboard::readUInt32(const char* buf) const
{
	return	(static_cast<UInt32>(buf[0]) << 24) |
			(static_cast<UInt32>(buf[1]) << 16) |
			(static_cast<UInt32>(buf[2]) <<  8) |
			 static_cast<UInt32>(buf[3]);
}

void					CClipboard::writeUInt32(CString* buf, UInt32 v) const
{
	*buf += static_cast<UInt8>((v >> 24) & 0xff);
	*buf += static_cast<UInt8>((v >> 16) & 0xff);
	*buf += static_cast<UInt8>((v >>  8) & 0xff);
	*buf += static_cast<UInt8>( v        & 0xff);
}
