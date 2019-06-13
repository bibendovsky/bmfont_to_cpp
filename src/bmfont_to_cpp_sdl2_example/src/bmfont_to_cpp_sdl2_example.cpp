#include <codecvt>
#include <cstdint>
#include <iostream>
#include <locale>
#include <memory>
#include <vector>
#include "SDL.h"
#include "bmf.h"


template<typename TObject, void (*TDeleter)(TObject*)>
struct SdlDeleter
{
	void operator()(
		TObject* object) const noexcept
	{
		TDeleter(object);
	}
};

template<typename TObject, void (*TDeleter)(TObject*)>
using SdlUPtr = std::unique_ptr<TObject, SdlDeleter<TObject, TDeleter>>;

using SdlWindowUPtr = SdlUPtr<SDL_Window, ::SDL_DestroyWindow>;
using SdlRendererUPtr = SdlUPtr<SDL_Renderer, ::SDL_DestroyRenderer>;
using SdlTextureUPtr = SdlUPtr<SDL_Texture, ::SDL_DestroyTexture>;
using SdlSurfaceUPtr = SdlUPtr<SDL_Surface, ::SDL_FreeSurface>;

using SdlTextures = std::vector<SdlTextureUPtr>;

using TextUtf8Samples = std::vector<std::string>;


const int window_width = 640;
const int window_height = 480;


SdlWindowUPtr sdl_window_;
SdlRendererUPtr sdl_renderer_;
SdlTextures sdl_textures_;


const TextUtf8Samples& get_text_utf8_samples()
{
	//
	// http://www.as8.it/type/basic_kerning_text.html
	//

	static const auto result = TextUtf8Samples
	{
		u8"(PRESS SPACE TO TOGGLE KERNING)",
		u8"",
		u8"Think about it, your unique designs ",
		u8"evaluated by experts in typography, ",
		u8"fabulous prizes, recognition and the ",
		u8"envy of your peers! Not only that, your ",
		u8"work will be offered for sale around ",
		u8"the world by the best alternative type ",
		u8"foundry going. And that would be ",
		u8"us...GarageFonts.",
	};

	return result;
}

bool create_sdl_window()
{
	::sdl_window_ = SdlWindowUPtr{::SDL_CreateWindow(
		"BMFont to CPP converter example",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		::window_width,
		::window_height,
		SDL_WINDOW_HIDDEN
	)};

	if (::sdl_window_ == nullptr)
	{
		std::cout << "ERROR: Failed to create SDL window. " << ::SDL_GetError() << std::endl;

		return false;
	}

	return true;
}

bool create_sdl_renderer()
{
	::sdl_renderer_ = SdlRendererUPtr{::SDL_CreateRenderer(
		::sdl_window_.get(),
		-1,
		0
	)};

	if (::sdl_renderer_ == nullptr)
	{
		std::cout << "ERROR: Failed to create SDL renderer. " << ::SDL_GetError() << std::endl;

		return false;
	}

	return true;
}

bool initialize_sdl()
{
	auto sdl_result = ::SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);

	if (sdl_result != 0)
	{
		std::cout << "ERROR: Failed to initialize SDL. " << ::SDL_GetError() << std::endl;

		return false;
	}

	return true;
}

SdlSurfaceUPtr create_sdl_surface()
{
	const Uint32 sdl_pixel_format =
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		SDL_PIXELFORMAT_RGBA8888
#else
		SDL_PIXELFORMAT_ABGR8888
#endif
;

	const auto& info = bmf2cpp::Font::get_info();

	auto sdl_surface = SdlSurfaceUPtr{::SDL_CreateRGBSurfaceWithFormat(
		0,
		info.page_width,
		info.page_height,
		32,
		sdl_pixel_format
	)};

	if (sdl_surface == nullptr)
	{
		std::cout << "ERROR: Failed to create SDL surface. " << ::SDL_GetError() << std::endl;
	}

	return sdl_surface;
}

void initialize_sdl_surface(
	const int page_index,
	SDL_Surface* sdl_surface)
{
	const auto& info = bmf2cpp::Font::get_info();
	const auto page = bmf2cpp::Font::get_page(page_index);

	const auto pitch = sdl_surface->pitch;

	auto page_offset = 0;
	auto dst_raw_line = static_cast<Uint8*>(sdl_surface->pixels);

	for (int h = 0; h < info.page_height; ++h)
	{
		auto dst_pixels = reinterpret_cast<Uint32*>(dst_raw_line);

		for (int w = 0; w < info.page_width; ++w)
		{
			const auto src_alpha = page[page_offset];

			const auto color = ::SDL_MapRGBA(
				sdl_surface->format,
				255,
				255,
				255,
				src_alpha
			);

			dst_pixels[w] = color;

			++page_offset;
		}

		dst_raw_line += pitch;
	}
}

bool create_sdl_texture(
	const int page_index)
{
	auto sdl_surface = create_sdl_surface();

	if (sdl_surface == nullptr)
	{
		return false;
	}

	initialize_sdl_surface(page_index, sdl_surface.get());

	const auto& info = bmf2cpp::Font::get_info();

	auto sdl_texture = SdlTextureUPtr{::SDL_CreateTextureFromSurface(
		::sdl_renderer_.get(),
		sdl_surface.get())
	};

	if (sdl_texture == nullptr)
	{
		std::cout << "ERROR: Failed to create SDL texture. " << ::SDL_GetError() << std::endl;

		return false;
	}

	::sdl_textures_[page_index] = std::move(sdl_texture);

	return true;
}

bool create_sdl_textures()
{
	const auto& info = bmf2cpp::Font::get_info();

	::sdl_textures_.resize(info.page_count);

	for (int i = 0; i < info.page_count; ++i)
	{
		if (!create_sdl_texture(i))
		{
			return false;
		}
	}

	return true;
}

#ifdef _MSC_VER
std::u32string to_utf32(
	const std::string& string_utf8)
{
	using StringUint32 = std::basic_string<std::uint32_t>;

	std::wstring_convert<std::codecvt_utf8<std::uint32_t>, std::uint32_t> converter;

	auto string_uint32 = converter.from_bytes(string_utf8);

	auto string_utf32 = std::u32string{
		reinterpret_cast<const char32_t*>(string_uint32.c_str()),
		string_uint32.size()
	};

	return string_utf32;
}
#else
std::u32string to_utf32(
	const std::string& string_utf8)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;

	return converter.from_bytes(string_utf8);
}
#endif // _MSC_VER

bool render_string(
	const int x,
	const int y,
	const bool use_kerning,
	const std::string& string_utf8)
{
	if (string_utf8.empty())
	{
		return true;
	}

	const auto& string_utf32 = to_utf32(string_utf8);
	const auto& font_info = bmf2cpp::Font::get_info();

	auto sdl_result = 0;
	auto is_first = true;
	auto cursor_x = 0;
	auto prev_ch = U'\0';

	for (const auto ch : string_utf32)
	{
		const auto glyp_info = bmf2cpp::Font::get_glyph(ch);

		if (glyp_info != nullptr)
		{
			if (is_first)
			{
				is_first = false;
				cursor_x -= glyp_info->offset_x;
			}

			if (use_kerning)
			{
				const auto kerning = bmf2cpp::Font::get_kerning(prev_ch, ch);

				cursor_x += kerning;
			}

			if (glyp_info->width > 0 && glyp_info->height)
			{
				const auto src_rect = SDL_Rect
				{
					glyp_info->page_x,
					glyp_info->page_y,
					glyp_info->width,
					glyp_info->height,
				};

				const auto dst_rect = SDL_Rect
				{
					x + cursor_x + glyp_info->offset_x,
					y + glyp_info->offset_y,
					glyp_info->width,
					glyp_info->height,
				};

				const auto& sdl_texture = ::sdl_textures_[glyp_info->page_id];

				sdl_result = ::SDL_RenderCopy(
					::sdl_renderer_.get(),
					sdl_texture.get(),
					&src_rect,
					&dst_rect
				);

				if (sdl_result != 0)
				{
					std::cout << "ERROR: Failed to render a glyph. " << ::SDL_GetError() << std::endl;

					return false;
				}
			}

			cursor_x += glyp_info->advance_x;
		}

		prev_ch = ch;
	}

	return true;
}

bool render_text_samples(
	const bool use_kerning)
{
	const auto& text_utf8_samples = get_text_utf8_samples();

	if (text_utf8_samples.empty())
	{
		std::cout << "ERROR: No text samples. " << std::endl;

		return false;
	}

	const auto& font_info = bmf2cpp::Font::get_info();

	const auto space_y = 1;

	auto x = 0;
	auto y = 0;

	for (const auto& text_utf8_sample : text_utf8_samples)
	{
		if (!render_string(x, y, use_kerning, text_utf8_sample))
		{
			return false;
		}

		y += font_info.line_height;
		y += space_y;
	}

	return true;
}

bool main_loop()
{
	const Uint32 sleep_ms = 50;

	auto sdl_result = 0;
	auto is_quit = false;
	auto sdl_event = SDL_Event{};

	::SDL_ShowWindow(::sdl_window_.get());

	sdl_result = ::SDL_SetRenderDrawBlendMode(
		::sdl_renderer_.get(),
		SDL_BLENDMODE_BLEND
	);

	if (sdl_result != 0)
	{
		std::cout << "ERROR: Failed to set SDL blend mode. " << ::SDL_GetError() << std::endl;

		return false;
	}

	auto use_kerning = false;

	while (!is_quit)
	{
		sdl_result = ::SDL_RenderClear(::sdl_renderer_.get());

		if (sdl_result != 0)
		{
			std::cout << "ERROR: Failed to clear SDL rendering target. " << ::SDL_GetError() << std::endl;

			return false;
		}

		if (!::render_text_samples(use_kerning))
		{
			return false;
		}

		::SDL_RenderPresent(::sdl_renderer_.get());


		while (::SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT)
			{
				is_quit = true;
			}

			if (sdl_event.type == SDL_KEYDOWN)
			{
				const auto mods = KMOD_CTRL | KMOD_ALT | KMOD_SHIFT;

				if ((sdl_event.key.keysym.mod & mods) == 0 &&
					sdl_event.key.keysym.scancode == SDL_SCANCODE_SPACE)
				{
					use_kerning = !use_kerning;
				}
			}
		}

		::SDL_Delay(sleep_ms);
	}

	return true;
}


int main(
	const int argc,
	char** argv)
{
	if (!::initialize_sdl())
	{
		return 1;
	}

	if (!::create_sdl_window())
	{
		return 1;
	}

	if (!::create_sdl_renderer())
	{
		return 1;
	}

	if (!::create_sdl_textures())
	{
		return 1;
	}

	if (!main_loop())
	{
		return 1;
	}

	return 0;
}
