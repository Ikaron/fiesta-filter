#pragma once

#include <vector>
#include <cstdint>
#include <span>
#include <string_view>

class PacketReader
{
private:
	const std::vector<char>& source;
	size_t read;
public:
	PacketReader(const std::vector<char>& source) : source(source), read(2) {}
	~PacketReader() {}

	PacketReader(const PacketReader& o) : source(o.source), read(o.read) {}
	PacketReader(PacketReader&& o) noexcept : source(o.source), read(o.read) {}
	PacketReader& operator=(const PacketReader& o) = delete;
	PacketReader& operator=(PacketReader&& o) = delete;

	void MoveToStart();
	bool MoveTo(size_t);
	size_t CurrentPos() const;
	bool Skip(intptr_t);

	size_t Remaining() const;

	bool IsOpcode(uint16_t) const;
	bool GetOpcode(uint16_t&) const;

	bool Read(void* dst, size_t length);

	template<typename T>
	bool Read(std::span<T> span)
	{
		auto byteSpan = std::as_writable_bytes(span);
		return Read(const_cast<std::byte*>(byteSpan.data()), byteSpan.size_bytes());
	}

	template<typename T>
	bool Read(T& target)
	{
		return Read(reinterpret_cast<void*>(&target), sizeof(T));
	}

	bool Read(uint8_t&);
	bool Read(uint16_t&);
	bool Read(uint32_t&);
	bool Read(uint64_t&);
	bool Read(int8_t&);
	bool Read(int16_t&);
	bool Read(int32_t&);
	bool Read(int64_t&);
	bool Read(float&);
	bool Read(double&);
	bool ReadCStr(std::string&);
	bool ReadShortStr(std::string&);
	bool ReadFixedStr(std::string&, size_t);
};

class PacketWriter
{
private:
	std::vector<char> buffer;
public:
	PacketWriter(size_t expectedSize = 0) : buffer({ 0, 0 })
	{
		if (expectedSize > 0)
			buffer.reserve(expectedSize);
	}

	~PacketWriter() {}

	PacketWriter(const PacketWriter& o) : buffer(o.buffer) {}
	PacketWriter(PacketWriter&& o) noexcept : buffer(std::move(o.buffer)) {}
	PacketWriter& operator=(const PacketWriter& o)
	{
		buffer = o.buffer;
		return *this;
	}

	PacketWriter& operator=(PacketWriter&& o) noexcept
	{
		buffer = std::move(o.buffer);
		return *this;
	}

	void IncreaseSize(size_t);
	void EnsureSize(size_t);

	void WriteZeroes(size_t);

	size_t DataLength() const;
	size_t TotalLength() const;

	void SetOpcode(uint16_t);

	void Write(const void* src, size_t length);

	template<typename T>
	void Write(std::span<T> span)
	{
		auto byteSpan = std::as_bytes(span);
		Write(byteSpan.data(), byteSpan.size_bytes());
	}

	template<typename T>
	void Write(const T& target)
	{
		Write(reinterpret_cast<const void*>(&target), sizeof(T));
	}

	void Write(uint8_t);
	void Write(uint16_t);
	void Write(uint32_t);
	void Write(uint64_t);
	void Write(int8_t);
	void Write(int16_t);
	void Write(int32_t);
	void Write(int64_t);
	void Write(float);
	void Write(double);
	void WriteRawStr(std::string_view);
	void WriteCStr(std::string_view);
	void WriteShortStr(std::string_view);
	void WriteFixedStr(std::string_view, size_t);

	void CopyTo(std::vector<char>&);
	void AppendTo(std::vector<char>&);
	const std::vector<char>& GetBuffer() const;
};