#include "FilterLib/Packet.h"

#include <string>

void PacketReader::MoveToStart()
{
	read = 2;
}

bool PacketReader::MoveTo(size_t pos)
{
	if (pos < 2 || pos >= source.size())
		return false;

	read = pos;
	return true;
}

size_t PacketReader::CurrentPos() const
{
	return read;
}

bool PacketReader::Skip(intptr_t toSkip)
{
	if (toSkip > 0 && Remaining() < toSkip)
		return false;
	if (toSkip < 0 && read + toSkip < 2)
		return false;

	read += toSkip;
	return true;
}

bool PacketReader::IsOpcode(uint16_t targetOpcode) const
{
	uint16_t actualOpcode;
	if (!GetOpcode(actualOpcode))
		return false;
	return actualOpcode == targetOpcode;
}

bool PacketReader::GetOpcode(uint16_t& out) const
{
	out = 0;

	if (source.size() < 2)
		return false;

	out |= (((uint16_t)source[0]) & 0xFF);
	out |= (((uint16_t)source[1]) & 0xFF) << 8;
	return true;
}

size_t PacketReader::Remaining() const
{
	return source.size() - read;
}

bool PacketReader::Read(void* dst, size_t length)
{
	if (Remaining() < length)
		return false;

	memcpy(dst, source.data() + read, length);
	read += length;
	return true;
}

bool PacketReader::Read(uint8_t& out)
{
	out = 0;

	if (Remaining() < sizeof(uint8_t))
		return false;

	out |= source[read++];
	return true;
}

bool PacketReader::Read(uint16_t& out)
{
	out = 0;

	if (Remaining() < sizeof(uint16_t))
		return false;

	out |= (((uint16_t)source[read++]) & 0xFF);
	out |= (((uint16_t)source[read++]) & 0xFF) << 8;
	return true;
}

bool PacketReader::Read(uint32_t& out)
{
	out = 0;

	if (Remaining() < sizeof(uint32_t))
		return false;

	out |= (((uint32_t)source[read++]) & 0xFF);
	out |= (((uint32_t)source[read++]) & 0xFF) << 8;
	out |= (((uint32_t)source[read++]) & 0xFF) << 16;
	out |= (((uint32_t)source[read++]) & 0xFF) << 24;
	return true;
}

bool PacketReader::Read(uint64_t& out)
{
	out = 0;

	if (Remaining() < sizeof(uint64_t))
		return false;

	out |= (((uint64_t)source[read++]) & 0xFF);
	out |= (((uint64_t)source[read++]) & 0xFF) << 8;
	out |= (((uint64_t)source[read++]) & 0xFF) << 16;
	out |= (((uint64_t)source[read++]) & 0xFF) << 24;
	out |= (((uint64_t)source[read++]) & 0xFF) << 32;
	out |= (((uint64_t)source[read++]) & 0xFF) << 40;
	out |= (((uint64_t)source[read++]) & 0xFF) << 48;
	out |= (((uint64_t)source[read++]) & 0xFF) << 56;
	return true;
}

bool PacketReader::Read(int8_t& out)
{
	out = 0;

	if (Remaining() < sizeof(int8_t))
		return false;

	out |= source[read++];
	return true;
}

bool PacketReader::Read(int16_t& out)
{
	out = 0;

	if (Remaining() < sizeof(int16_t))
		return false;

	out |= source[read++];
	out |= ((int16_t)source[read++]) << 8;
	return true;
}

bool PacketReader::Read(int32_t& out)
{
	out = 0;

	if (Remaining() < sizeof(int32_t))
		return false;

	out |= source[read++];
	out |= ((int32_t)source[read++]) << 8;
	out |= ((int32_t)source[read++]) << 16;
	out |= ((int32_t)source[read++]) << 24;
	return true;
}

bool PacketReader::Read(int64_t& out)
{
	out = 0;

	if (Remaining() < sizeof(int64_t))
		return false;

	out |= source[read++];
	out |= ((int64_t)source[read++]) << 8;
	out |= ((int64_t)source[read++]) << 16;
	out |= ((int64_t)source[read++]) << 24;
	out |= ((int64_t)source[read++]) << 32;
	out |= ((int64_t)source[read++]) << 40;
	out |= ((int64_t)source[read++]) << 48;
	out |= ((int64_t)source[read++]) << 56;
	return true;
}

bool PacketReader::Read(float& out)
{
	out = 0;
	return Read(&out, sizeof(float));
}

bool PacketReader::Read(double& out)
{
	out = 0;
	return Read(&out, sizeof(double));
}

bool PacketReader::ReadCStr(std::string& out)
{
	out.resize(0);

	if (Remaining() == 0)
		return false;
	
	size_t length = strnlen_s(source.data() + read, Remaining());
	if (length == 0)
		return true;

	if (length == Remaining()) // TODO: Check if this is correct
		return false;

	out.resize(length);
	memcpy(out.data(), source.data() + read, length);
	read += length;

	return false;
}

bool PacketReader::ReadShortStr(std::string& out)
{
	out.resize(0);

	uint8_t length;
	if (!Read(length))
		return false;

	if (Remaining() < length)
		return false;

	out.resize(length);
	memcpy(out.data(), source.data() + read, length);
	read += length;

	return true;
}

bool PacketReader::ReadFixedStr(std::string& out, size_t len)
{
	out.resize(0);

	if (Remaining() < len)
		return false;

	size_t length = strnlen_s(source.data() + read, Remaining());
	if (length == 0)
		return true;

	out.resize(length);
	memcpy(out.data(), source.data() + read, length);
	read += len;

	return true;
}

void PacketWriter::IncreaseSize(size_t count)
{
	EnsureSize(buffer.size() + count);
}

void PacketWriter::EnsureSize(size_t size)
{
	if(size > buffer.size())
		buffer.resize(size);
}

void PacketWriter::WriteZeroes(size_t count)
{
	buffer.resize(buffer.size() + count, 0);
}

size_t PacketWriter::TotalLength() const
{
	return buffer.size();
}

size_t PacketWriter::DataLength() const
{
	auto totalLength = TotalLength();
	if (totalLength < 2)
		return 0;
	return totalLength - 2;
}


void PacketWriter::SetOpcode(uint16_t opcode)
{
	EnsureSize(2);
	buffer[0] = (char) opcode;
	buffer[1] = (char) (opcode >> 8);
}


void PacketWriter::Write(const void* src, size_t length)
{
	auto pos = TotalLength();
	IncreaseSize(length);
	memcpy(buffer.data() + pos, src, length);
}

void PacketWriter::Write(uint8_t val)
{
	buffer.push_back((char) val);
}

void PacketWriter::Write(uint16_t val)
{
	buffer.push_back((char) val);
	buffer.push_back((char) (val >> 8));
}

void PacketWriter::Write(uint32_t val)
{
	buffer.push_back((char) val);
	buffer.push_back((char) (val >> 8));
	buffer.push_back((char) (val >> 16));
	buffer.push_back((char) (val >> 24));
}

void PacketWriter::Write(uint64_t val)
{
	buffer.push_back((char) val);
	buffer.push_back((char) (val >> 8));
	buffer.push_back((char) (val >> 16));
	buffer.push_back((char) (val >> 24));
	buffer.push_back((char) (val >> 32));
	buffer.push_back((char) (val >> 40));
	buffer.push_back((char) (val >> 48));
	buffer.push_back((char) (val >> 56));
}

void PacketWriter::Write(int8_t val)
{
	Write((uint8_t) val);
}

void PacketWriter::Write(int16_t val)
{
	Write((uint16_t) val);
}

void PacketWriter::Write(int32_t val)
{
	Write((uint32_t) val);
}

void PacketWriter::Write(int64_t val)
{
	Write((uint64_t) val);
}

void PacketWriter::Write(float val)
{
	Write<float>(val);
}

void PacketWriter::Write(double val)
{
	Write<double>(val);
}

void PacketWriter::WriteCStr(std::string_view val)
{
	Write(val.data(), val.length());
	Write((uint8_t) 0);
}

void PacketWriter::WriteShortStr(std::string_view val)
{
	if (val.length() > 0xff)
		throw;

	Write((uint8_t) val.length());
	Write(val.data(), val.length());
}

void PacketWriter::WriteFixedStr(std::string_view val, size_t len)
{
	if (val.length() > len)
		throw;

	Write(val.data(), val.length());
	size_t remaining = len - val.length();
	if (remaining > 0)
		WriteZeroes(remaining);
}

void PacketWriter::CopyTo(std::vector<char>& out)
{
	out.resize(buffer.size());
	memcpy(out.data(), buffer.data(), buffer.size());
}

const std::vector<char>& PacketWriter::GetBuffer() const
{
	return buffer;
}