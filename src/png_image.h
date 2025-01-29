#pragma once

#include <meta/object.h>
#include <common/intrusive_ptr.h>
#include <uv/buffer.h>
#include <png.h>
#include "image.h"

namespace lua {
	class state;
	class multiret;
}

class PNGImage : public meta::object {
	META_OBJECT
private:
	png_structp m_write;
	png_infop	m_info;
	ImageFormat m_format = ImageFormat::GRAY;
	static void error_fn(png_structp png_ptr,
        png_const_charp error_msg);
	static void warning_fn(png_structp png_ptr,
        png_const_charp error_msg);
	size_t m_width = 0;
	size_t m_height = 0;
	static void write_fn(png_structp, png_bytep, size_t);
	static void flush_fn(png_structp);
	uv::buffer_ptr m_png_data;
	void write(png_bytep, size_t);
	bool write_header(ImageFormat fmt,size_t w,size_t h);
	bool write_data_buffer(const uv::buffer_base_ptr& data);
	uv::buffer_ptr end_write();
public:
	PNGImage();
	~PNGImage();
	static void lbind(lua::state& l);
	static lua::multiret lnew(lua::state& l);
	static uv::buffer_ptr do_encode(const ImagePtr& img);
	static ImagePtr do_decode(const uv::buffer_base_ptr& data);

	static lua::multiret encode(lua::state& l);
	static lua::multiret decode(lua::state& l);

	void set_size(lua::state& l);
	void write_row(lua::state& l);
	void write_data(lua::state& l);
	lua::multiret close(lua::state& l);
};
typedef common::intrusive_ptr<PNGImage> PNGImagePtr; 
