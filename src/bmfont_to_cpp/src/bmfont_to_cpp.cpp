/*

BMFont to CPP header converter

Converts bitmap fonts generated with Bitmap Font Generator
(http://www.angelcode.com/products/bmfont/) into C++ static data

Copyright (c) 2014-2019 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/


/*
Program requirements:
    C++11 compatible compiler.
    CMake 3.5.1 (optional).

Font requirements:
    .FNT format - text
    .FNT channel configuration:
        1) R:3 G:3 B:3 A:0
        2) R:4 G:4 B:4 A:0
    image format - DDS (alpha, 8 bit)
*/


#include <cstdint>
#include <array>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>


// ========================================================================
// Module template
//

/*
namespace bmf2cpp
{


struct FontInfo
{
	int font_size;
	int line_height;
	int base_offset;
	int page_count;
	int page_width;
	int page_height;
}; // FontInfo

struct GlyphInfo
{
	int page_id;
	int page_x;
	int page_y;
	int width;
	int height;
	int offset_x;
	int offset_y;
	int advance_x;
}; // GlyphInfo

class Font
{
public:
	Font() = delete;

	Font(
		const Font& that) = delete;

	Font& operator=(
		const Font& that) = delete;

	~Font() = delete;

	static const FontInfo& get_info();

	static const GlyphInfo* get_glyph(
		const char32_t index);

	static int get_kerning(
		const char32_t left_char,
		const char32_t right_char);

	static const unsigned char* get_page(
		const int page_index);
}; // Font

const FontInfo& Font::get_info()
{
	static FontInfo font_info =
	{
		...
	}; // font_info

	return font_info;
}

const GlyphInfo* Font::get_glyph(
	const char32_t index)
{
	using Glyphs = std::unordered_map<char32_t, GlyphInfo>;

	static const Glyphs glyphs =
	{
		...
	}; // glyphs

	auto glyph_it = glyphs.find(index);

	if (glyph_it == glyphs.cend())
	{
		return nullptr;
	}

	return &glyph_it->second;
}

int Font::get_kerning(
	const char32_t left_char,
	const char32_t right_char)
{
	using Kernings = std::unordered_map<
		char32_t,
		std::unordered_map<char32_t, int>>;

	static const Kernings kernings =
	{
		...
	}; // kernings

	if (left_char == '\0' || right_char == '\0')
	{
		return 0;
	}

	auto sub_kerning_it = kernings.find(left_char);

	if (sub_kerning_it == kernings.cend())
	{
		return 0;
	}

	const auto& sub_kerning = sub_kerning_it->second;

	auto kerning_it = sub_kerning.find(right_char);

	if (kerning_it == sub_kerning.cend())
	{
		return 0;
	}

	return kerning_it->second;
}

const unsigned char* Font::get_page(
	const int page_index)
{
	using Pages = std::array<std::array<unsigned char, ?>, ?>;

	static const Pages pages =
	{{
		...
	}}; // pages

	return pages[page_index].data();
}


} // namespace bmf2cpp
*/

//
// Template
// ========================================================================


// ========================================================================
// DDS stuff

struct DDS_PIXELFORMAT
{
	std::uint32_t dwSize;
	std::uint32_t dwFlags;
	std::uint32_t dwFourCC;
	std::uint32_t dwRGBBitCount;
	std::uint32_t dwRBitMask;
	std::uint32_t dwGBitMask;
	std::uint32_t dwBBitMask;
	std::uint32_t dwABitMask;
}; // DDS_PIXELFORMAT


const size_t DDS_HEADER_SIZE = 124;

struct DDS_HEADER
{
	std::uint32_t dwSize;
	std::uint32_t dwFlags;
	std::uint32_t dwHeight;
	std::uint32_t dwWidth;
	std::uint32_t dwPitchOrLinearSize;
	std::uint32_t dwDepth;
	std::uint32_t dwMipMapCount;
	std::uint32_t dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	std::uint32_t dwCaps;
	std::uint32_t dwCaps2;
	std::uint32_t dwCaps3;
	std::uint32_t dwCaps4;
	std::uint32_t dwReserved2;
}; // DDS_HEADER

std::uint32_t MAKEFOURCC(
	std::uint8_t a,
	std::uint8_t b,
	std::uint8_t c,
	std::uint8_t d)
{
	return
		(static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) << 0) |
		(static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 8) |
		(static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 16) |
		(static_cast<std::uint32_t>(static_cast<std::uint8_t>(d)) << 24);
}

// DDS stuff
// ========================================================================


bool is_pow2(
	const int x)
{
	if (x < 0)
	{
		return false;
	}

	if (x <= 1)
	{
		return true;
	}

	auto power = 0;

	for (auto y = x; y > 1; y /= 2)
	{
		power += 1;
	}

	return x == (1 << power);
}


using LineParts = std::unordered_map<std::string, std::string>;


LineParts parse_line(
	const std::string& line,
	const std::string& keyword)
{
	if (line.empty())
	{
		return LineParts{};
	}

	if (keyword.empty())
	{
		throw std::invalid_argument{"Empty keyword."};
	}

	enum class State
	{
		find_keyword_end,
		find_key_begin,
		find_key_end,
		find_equal_sign,
		find_value_begin,
		find_value_end,
		done,
	}; // State

	std::string key;
	std::string value;
	LineParts line_parts;

	auto state = State::find_keyword_end;

	auto is_string_value = false;
	auto char_it = line.cbegin();
	auto char_begin_it = line.cbegin();
	auto line_end_it = line.cend();

	while (state != State::done)
	{
		switch (state)
		{
			case State::find_keyword_end:
				while (char_it != line_end_it && (*char_it) != ' ')
				{
					++char_it;
				}

				key.assign(char_begin_it, char_it);

				if (key != keyword)
				{
					const std::string error_message = "Keyword not found: \"" + keyword + "\"";

					throw std::runtime_error{error_message};
				}

				state = State::find_key_begin;
				break;

			case State::find_key_begin:
				while (char_it != line_end_it && (*char_it) == ' ')
				{
					++char_it;
				}

				if (char_it == line_end_it)
				{
					state = State::done;
				}
				else
				{
					char_begin_it = char_it;
					state = State::find_key_end;
				}
				break;

			case State::find_key_end:
				while (char_it != line_end_it && (*char_it) != ' ' && (*char_it) != '=')
				{
					++char_it;
				}

				key.assign(char_begin_it, char_it);

				if (key.empty())
				{
					throw std::runtime_error{"Empty key."};
				}

				state = State::find_equal_sign;
				break;

			case State::find_equal_sign:
				while (char_it != line_end_it && (*char_it) != '=')
				{
					++char_it;
				}

				if (char_it == line_end_it)
				{
					throw std::runtime_error{"Equal sign expected."};
				}

				++char_it;
				state = State::find_value_begin;
				break;

			case State::find_value_begin:
				while (char_it != line_end_it && (*char_it) == ' ')
				{
					++char_it;
				}

				if (char_it == line_end_it)
				{
					throw std::runtime_error{"Value expected."};
				}

				char_begin_it = char_it;
				is_string_value = ((*char_it) == '\"');
				++char_it;
				state = State::find_value_end;
				break;

			case State::find_value_end:
				while (char_it != line_end_it)
				{
					if ((*char_it) == ' ')
					{
						if (!is_string_value)
						{
							break;
						}
					}
					else if (is_string_value && (*char_it) == '\"')
					{
						break;
					}

					++char_it;
				}

				if (is_string_value)
				{
					if (char_it == line_end_it)
					{
						throw std::runtime_error{"Unexpected end of string value."};
					}
					else
					{
						++char_it;
					}
				}

				if (is_string_value)
				{
					value.assign(char_begin_it + 1, char_it - 1);
				}
				else
				{
					value.assign(char_begin_it, char_it);
				}

				line_parts[key] = value;
				state = State::find_key_begin;
				break;

			case State::done:
				break;

			default:
				throw std::runtime_error{"Invalid state."};
		}
	}

	return line_parts;
}


struct CharInfo
{
	char32_t id;
	int x;
	int y;
	int width;
	int height;
	int xoffset;
	int yoffset;
	int xadvance;
	int page;
	int chnl;
}; // CharInfo


struct Page
{
	using Data = std::vector<char>;


	int id;
	std::string file;
	Data data;


	void read_data(
		const int width,
		const int height)
	{
		std::ifstream stream{file, std::ios_base::in | std::ios_base::binary};

		if (!stream.is_open())
		{
			std::string message = "Failed to open page: \"";
			message += file;
			message + "\".";

			throw std::runtime_error{message};
		}

		std::uint32_t dds_magic;
		stream.read(reinterpret_cast<char*>(&dds_magic), 4);

		if (dds_magic != MAKEFOURCC('D', 'D', 'S', ' '))
		{
			std::string message = "Page file is not DDS: \"";
			message += file;
			message + "\".";

			throw std::runtime_error{message};
		}


		DDS_HEADER dds_header;

		stream.read(reinterpret_cast<char*>(&dds_header), DDS_HEADER_SIZE);

		if (dds_header.dwSize != DDS_HEADER_SIZE)
		{
			std::string message = "Invalid DDS header size: \"";
			message += file;
			message + "\".";

			throw std::runtime_error{message};
		}

		if (dds_header.ddspf.dwRGBBitCount != 8 ||
			dds_header.ddspf.dwABitMask != 255)
		{
			std::string message = "Unsupported image format: \"";
			message += file;
			message + "\".";

			throw std::runtime_error{message};
		}

		auto data_size = width * height;
		data.resize(data_size);

		stream.read(data.data(), data_size);

		if (!stream || stream.gcount() != data_size)
		{
			std::string message = "Failed to read page data: \"";
			message += file;
			message + "\".";

			throw std::runtime_error{message};
		}
	}
}; // Page


class FntInfo
{
public:
	using Pages = std::vector<Page>;
	using Chars = std::vector<CharInfo>;
	using SubKernings = std::map<char32_t, int>;
	using Kernings = std::map<char32_t, SubKernings>;


	std::string face;
	int size;
	//int bold;
	//int italic;
	//int charset;
	//int unicode;
	int stretchH;
	//int smooth;
	//int aa;
	//int padding[4];
	//int spacing[2];
	int outline;

	int lineHeight;
	int base;
	int scaleW;
	int scaleH;
	int pages;
	int packed;
	int alphaChnl;
	int redChnl;
	int greenChnl;
	int blueChnl;

	Pages page_list;
	Chars chars;
	Kernings kernings;


	FntInfo()
	{
	}

	FntInfo(
		const FntInfo& that) = delete;

	FntInfo& operator=(
		const FntInfo& that) = delete;

	~FntInfo()
	{
	}


	void parse(
		std::istream& stream)
	{
		std::string line;


		// info

		if (!std::getline(stream, line))
		{
			throw std::runtime_error{"Failed to read a line."};
		}

		auto info_parts = parse_line(line, "info");

		face = info_parts["face"];
		size = std::stoi(info_parts["size"]);
		stretchH = std::stoi(info_parts["stretchH"]);
		outline = std::stoi(info_parts["outline"]);

		if (size >= 0)
		{
			throw std::runtime_error{"Positive size."};
		}

		if (stretchH != 100)
		{
			throw std::runtime_error{"Stretch is not 100%."};
		}

		if (outline != 0)
		{
			throw std::runtime_error{"Outline not allowed."};
		}


		// common

		if (!std::getline(stream, line))
		{
			throw std::runtime_error{"Failed to read a line."};
		}

		auto common_parts = parse_line(line, "common");

		lineHeight = std::stoi(common_parts["lineHeight"]);
		base = std::stoi(common_parts["base"]);
		scaleW = std::stoi(common_parts["scaleW"]);
		scaleH = std::stoi(common_parts["scaleH"]);
		pages = std::stoi(common_parts["pages"]);
		packed = std::stoi(common_parts["packed"]);
		alphaChnl = std::stoi(common_parts["alphaChnl"]);
		redChnl = std::stoi(common_parts["redChnl"]);
		greenChnl = std::stoi(common_parts["greenChnl"]);
		blueChnl = std::stoi(common_parts["blueChnl"]);

		if (base <= 0)
		{
			throw std::runtime_error{"Invalid base."};
		}

		if (!is_pow2(scaleW) || !is_pow2(scaleH))
		{
			throw std::runtime_error{"Non power of two dimensions."};
		}

		if (pages <= 0)
		{
			throw std::runtime_error{"Invalid page count."};
		}

		if (packed != 0)
		{
			throw std::runtime_error{"Packed data not supported."};
		}

		auto is_channels_valid = false;

		if (!is_channels_valid)
		{
			is_channels_valid |=
				alphaChnl == 0 &&
				redChnl == 3 &&
				greenChnl == 3 &&
				blueChnl == 3;
		}

		if (!is_channels_valid)
		{
			is_channels_valid |=
				alphaChnl == 0 &&
				redChnl == 4 &&
				greenChnl == 4 &&
				blueChnl == 4;
		}

		if (!is_channels_valid)
		{
			throw std::runtime_error{"Unexpected channel configuration."};
		}

		page_list.resize(pages);

		for (auto i = 0; i < pages; ++i)
		{
			// page

			if (!std::getline(stream, line))
			{
				throw std::runtime_error{"Failed to read a line."};
			}

			auto page_parts = parse_line(line, "page");

			auto& page = page_list[i];
			page.id = std::stoi(page_parts["id"]);
			page.file = page_parts["file"];

			page.read_data(scaleW, scaleH);
		}


		if (!std::getline(stream, line))
		{
			throw std::runtime_error{"Failed to read a line."};
		}

		auto chars_parts = parse_line(line, "chars");
		auto char_count = std::stoi(chars_parts["count"]);

		chars.resize(char_count);

		for (auto i = 0; i < char_count; ++i)
		{
			// char

			if (!std::getline(stream, line))
			{
				throw std::runtime_error{"Failed to read a line."};
			}

			auto char_parts = parse_line(line, "char");

			auto& ch = chars[i];

			ch.id = std::stoul(char_parts["id"]);
			ch.x = std::stoi(char_parts["x"]);
			ch.y = std::stoi(char_parts["y"]);
			ch.width = std::stoi(char_parts["width"]);
			ch.height = std::stoi(char_parts["height"]);
			ch.xoffset = std::stoi(char_parts["xoffset"]);
			ch.yoffset = std::stoi(char_parts["yoffset"]);
			ch.xadvance = std::stoi(char_parts["xadvance"]);
			ch.page = std::stoi(char_parts["page"]);
			ch.chnl = std::stoi(char_parts["chnl"]);
		}


		// Kernings

		if (!std::getline(stream, line))
		{
			return;
		}

		auto kernings_parts = parse_line(line, "kernings");
		auto kerning_count = std::stoi(kernings_parts["count"]);

		for (auto i = 0; i < kerning_count; ++i)
		{
			// kerning

			if (!std::getline(stream, line))
			{
				throw std::runtime_error{"Failed to read a line."};
			}

			auto kerning_parts = parse_line(line, "kerning");
			auto first = std::stoi(kerning_parts["first"]);
			auto second = std::stoi(kerning_parts["second"]);
			auto amount = std::stoi(kerning_parts["amount"]);

			auto first_it = kernings.find(first);

			if (first_it == kernings.end())
			{
				kernings[first] = SubKernings();
				first_it = kernings.find(first);
			}

			auto& sub_kernings = first_it->second;
			sub_kernings[second] = amount;
		}
	}

	void export_to_cpp(
		const std::string& file_name)
	{
		std::ofstream stream(file_name);

		if (!stream.is_open())
		{
			std::string error_message = "Failed to open cpp file: \"";
			error_message += file_name;
			error_message += "\"";

			throw std::runtime_error{error_message};
		}

		auto data_size = scaleH * scaleW;

		stream <<
			"//" << std::endl <<
			"// Generated by application bmfont_to_cpp." << std::endl <<
			"//" << std::endl <<
			std::endl <<
			std::endl <<
			"#include <array>" << std::endl <<
			"#include <unordered_map>" << std::endl <<
			std::endl <<
			std::endl <<
			"namespace bmf2cpp" << std::endl <<
			"{" << std::endl <<
			std::endl <<
			std::endl <<
			"struct FontInfo" << std::endl <<
			"{" << std::endl <<
			"\tint font_size;" << std::endl <<
			"\tint line_height;" << std::endl <<
			"\tint base_offset;" << std::endl <<
			"\tint page_count;" << std::endl <<
			"\tint page_width;" << std::endl <<
			"\tint page_height;" << std::endl <<
			"}; // FontInfo" << std::endl <<
			std::endl <<
			"struct GlyphInfo" << std::endl <<
			"{" << std::endl <<
			"\tint page_id;" << std::endl <<
			"\tint page_x;" << std::endl <<
			"\tint page_y;" << std::endl <<
			"\tint width;" << std::endl <<
			"\tint height;" << std::endl <<
			"\tint offset_x;" << std::endl <<
			"\tint offset_y;" << std::endl <<
			"\tint advance_x;" << std::endl <<
			"}; // GlyphInfo" << std::endl <<
			std::endl <<
			std::endl <<
			"struct Font" << std::endl <<
			"{" << std::endl <<
			"\tFont() = delete;" << std::endl <<
			std::endl <<
			"\tFont(" << std::endl <<
			"\t\tconst Font& that) = delete;" << std::endl <<
			std::endl <<
			"\tFont& operator=(" << std::endl <<
			"\t\tconst Font& that) = delete;" << std::endl <<
			std::endl <<
			"\t~Font() = delete;" << std::endl <<
			std::endl <<
			"\tstatic const FontInfo& get_info();" << std::endl <<
			std::endl <<
			"\tstatic const GlyphInfo* get_glyph(" << std::endl <<
			"\t\tconst char32_t index);" << std::endl <<
			std::endl <<
			"\tstatic int get_kerning(" << std::endl <<
			"\t\tconst char32_t left_char," << std::endl <<
			"\t\tconst char32_t right_char);" << std::endl <<
			std::endl <<
			"\tstatic const unsigned char* get_page(" << std::endl <<
			"\t\tconst int page_index);" << std::endl <<
			"}; // Font" << std::endl <<
			std::endl <<
			std::endl <<
			"const FontInfo& Font::get_info()" << std::endl <<
			"{" << std::endl <<
			"\tstatic FontInfo font_info = {" << std::endl <<
			"\t\t" <<
			size << ", " <<
			lineHeight << ", " <<
			base << ", " <<
			page_list.size() << ", " <<
			scaleW << ", " <<
			scaleH << std::endl <<
			"\t}; // font_info" << std::endl <<
			std::endl <<
			"\treturn font_info;" << std::endl <<
			"}" << std::endl;


		//
		// Glyphs
		//

		stream <<
			std::endl <<
			std::endl <<
			"const GlyphInfo* Font::get_glyph(" << std::endl <<
			"\tconst char32_t index)" << std::endl <<
			"{" << std::endl <<
			"\tusing Glyphs = std::unordered_map<char32_t, GlyphInfo>;" << std::endl <<
			std::endl <<
			"\tstatic const Glyphs glyphs = {" << std::endl;

		for (size_t i = 0; i < chars.size(); ++i)
		{
			const auto& ch = chars[i];

			stream <<
				"\t\t{ " <<
				ch.id << ", { " <<
				ch.page << ", " <<
				ch.x << ", " <<
				ch.y << ", " <<
				ch.width << ", " <<
				ch.height << ", " <<
				ch.xoffset << ", " <<
				ch.yoffset << ", " <<
				ch.xadvance << ", " <<
				" } }," << std::endl;
		}

		stream <<
			"\t}; // glyphs" << std::endl <<
			std::endl <<
			"\tauto glyph_it = glyphs.find(index);" << std::endl <<
			std::endl <<
			"\tif (glyph_it == glyphs.cend())" << std::endl <<
			"\t{" << std::endl <<
			"\t\treturn nullptr;" << std::endl <<
			"\t}" << std::endl <<
			std::endl <<
			"\treturn &glyph_it->second;" << std::endl <<
			"}" << std::endl;


		//
		// Kernings
		//

		stream <<
			std::endl <<
			std::endl <<
			"int Font::get_kerning(" << std::endl <<
			"\tchar32_t left_char," << std::endl <<
			"\tchar32_t right_char)" << std::endl <<
			"{" << std::endl <<
			"\tusing Kernings = std::unordered_map<" << std::endl <<
			"\t\tchar32_t," << std::endl <<
			"\t\tstd::unordered_map<char32_t, int>>;" << std::endl <<
			std::endl <<
			"\tstatic const Kernings kernings = {" << std::endl;

		for (const auto& kerning : kernings)
		{
			stream <<
				"\t\t{" << std::endl <<
				"\t\t\t" << kerning.first << "," << std::endl <<
				"\t\t\t{" << std::endl;

			for (const auto& sub_kerning : kerning.second)
			{
				stream <<
					"\t\t\t\t{ " << sub_kerning.first << ", " <<
					sub_kerning.second << " }," << std::endl;
			}

			stream <<
				"\t\t\t}" << std::endl <<
				"\t\t}," << std::endl;
		}

		stream <<
			"\t}; // kernings" << std::endl <<
			std::endl <<
			"\tif (left_char == '\\0' || right_char == '\\0')" << std::endl <<
			"\t{" << std::endl <<
			"\t\treturn 0;" << std::endl <<
			"\t}" << std::endl <<
			std::endl <<
			"\tauto sub_kerning_it = kernings.find(left_char);" << std::endl <<
			std::endl <<
			"\tif (sub_kerning_it == kernings.cend())" << std::endl <<
			"\t{" << std::endl <<
			"\t\treturn 0;" << std::endl <<
			"\t}" << std::endl <<
			std::endl <<
			"\tconst auto& sub_kerning = sub_kerning_it->second;" << std::endl <<
			std::endl <<
			"\tauto kerning_it = sub_kerning.find(right_char);" << std::endl <<
			std::endl <<
			"\tif (kerning_it == sub_kerning.cend())" << std::endl <<
			"\t{" << std::endl <<
			"\t\treturn 0;" << std::endl <<
			"\t}" << std::endl <<
			std::endl <<
			"\treturn kerning_it->second;" << std::endl <<
			"}" << std::endl;


		//
		// Pages
		//

		stream <<
			std::endl <<
			std::endl <<
			"const unsigned char* Font::get_page(" << std::endl <<
			"\tconst int page_index)" << std::endl <<
			"{" << std::endl <<
			"\tusing Pages = std::array<std::array<unsigned char, " <<
			data_size << ">, " << page_list.size() << ">;" << std::endl <<
			std::endl <<
			"\tstatic const Pages pages = {{" << std::endl;

		const int octets_per_line = 11;

		for (auto i = 0; i < pages; ++i)
		{
			stream << "\t\t{" << std::endl;

			stream << std::setfill('0') << std::uppercase;

			const auto& page = page_list[i];
			const auto& data = page.data;

			for (auto j = 0; j < data_size; ++j)
			{
				const auto& o = data[j];

				auto is_first_octet = ((j % octets_per_line) == 0);
				auto is_new_line = ((j % octets_per_line) == (octets_per_line - 1));
				auto is_last_octet = ((j + 1) == data_size);

				if (is_first_octet)
				{
					stream << "\t\t\t";
				}

				stream << "0x" << std::hex << std::setw(2) <<
					static_cast<int>(static_cast<unsigned char>(o)) << std::dec << ',';

				if (!is_last_octet)
				{
					if (!is_new_line)
					{
						stream << ' ';
					}
					else
					{
						stream << std::endl;
					}
				}
				else
				{
					stream << std::endl;
				}
			}

			stream << "\t\t}," << std::endl;
		}

		stream <<
			"\t}}; // pages" << std::endl <<
			std::endl <<
			"\treturn pages[page_index].data();" << std::endl <<
			"}" << std::endl;


		// namespace closing
		stream <<
			std::endl <<
			std::endl <<
			"} // bmf2cpp" << std::endl;
	}
}; // FntInfo


int main(
	int argc,
	char** argv)
{
	if (argc != 3)
	{
		std::cout <<
			"BMFont to CPP converter" << std::endl <<
			std::endl <<
			"Font requirements:" << std::endl <<
			"    .FNT format - text" << std::endl <<
			"    .FNT channel configuration:" << std::endl <<
			"        1) R:3 G:3 B:3 A:0" << std::endl <<
			"        2) R:4 G:4 B:4 A:0" << std::endl <<
			"    image format - DDS (alpha, 8 bit)" << std::endl <<
			std::endl <<
			std::endl <<
			"Usage:" << std::endl <<
			std::endl <<
			"app.exe <fnt_file_name> <out_file_name>" << std::endl <<
			std::endl;
		return 1;
	}

	std::string fnt_file_name(argv[1]);
	std::ifstream fnt_stream(fnt_file_name);

	if (!fnt_stream.is_open())
	{
		std::cout << "Failed to open .fnt file: \"" << fnt_file_name << "\"" << std::endl;
		return 2;
	}


	try
	{
		FntInfo fnt_info;
		fnt_info.parse(fnt_stream);
		fnt_info.export_to_cpp(argv[2]);

		size_t kerning_count = 0;

		for (const auto& left_code : fnt_info.kernings)
		{
			kerning_count += left_code.second.size();
		}

		std::cout << "Code points: " << fnt_info.chars.size() << std::endl;
		std::cout << "Kerning pairs: " << kerning_count << std::endl;
		std::cout << "Pages: " << fnt_info.pages << std::endl;
		std::cout << "Page size: " << fnt_info.scaleW << 'x' << fnt_info.scaleH << std::endl;
	}
	catch (const std::exception& ex)
	{
		std::cout << "ERROR: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
