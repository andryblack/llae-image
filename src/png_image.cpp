#include "png_image.h"
#include "lua/state.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "llae/buffer.h"
#include "uv/work.h"
#include "uv/luv.h"
#include "llae/async_bind.h"
#include <iostream>
#include <vector>
#include "image.h"

META_OBJECT_INFO(image::PNGImage,meta::object)

namespace image {

	void PNGImage::error_fn(png_structp png_ptr,
	        png_const_charp error_msg) {
		std::cerr << "png error " << error_msg << std::endl;
	}
	void PNGImage::warning_fn(png_structp png_ptr,
	    png_const_charp error_msg) {
		std::cerr << "png warning " << error_msg << std::endl;
	}
	PNGImage::PNGImage() : m_write(0),m_info(0) {
		m_write = png_create_write_struct(PNG_LIBPNG_VER_STRING,this,
			PNGImage::error_fn,
			PNGImage::warning_fn);
		m_info = png_create_info_struct(m_write);
		png_set_write_fn(m_write,this,&PNGImage::write_fn,&PNGImage::flush_fn);
	}

	PNGImage::~PNGImage() {
		png_destroy_write_struct(&m_write,&m_info);
	}

	llae::result<PNGImagePtr> PNGImage::lnew(int w,int h,ImageFormat fmt) {
		PNGImagePtr res(new PNGImage());
		if (!res->write_header(fmt,w,h)) {
			return llae::string_error::create("invalid format");
		}
		return llae::result<PNGImagePtr>(std::move(res));
	}

	void PNGImage::write_fn(png_structp write, png_bytep data, size_t size) {
		static_cast<PNGImage*>(png_get_io_ptr(write))->write(data,size);
	}

	void PNGImage::flush_fn(png_structp) {
		
	}

	void PNGImage::write(png_bytep data, size_t size) {
		size_t pos = 0;
		if (!m_png_data) {
			m_png_data = llae::buffer::alloc(size*2);
			m_png_data->set_len(0);
		} else {
			pos = m_png_data->get_len();
			m_png_data = m_png_data->realloc(pos+size*2);
		}
		memcpy(static_cast<uint8_t*>(m_png_data->get_base())+pos,data,size);
		m_png_data->set_len(pos+size);
	}

	bool PNGImage::write_header(ImageFormat fmt,size_t w,size_t h) {
		m_format = fmt;
		m_width = w;
	    m_height = h;
	    if (m_format == ImageFormat::GRAY) {
			png_set_IHDR(m_write,m_info,w,h,8,
				PNG_COLOR_TYPE_GRAY,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
		} else if (m_format == ImageFormat::RGB) {
			png_set_IHDR(m_write,m_info,w,h,8,
				PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
		} else if (m_format == ImageFormat::RGBA) {
			png_set_IHDR(m_write,m_info,w,h,8,
				PNG_COLOR_TYPE_RGB_ALPHA,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
		} else {
			return false;
		}
		png_write_info(m_write,m_info);
		return true;
	}

	void PNGImage::write_row(lua::state& l) {
		auto buf = llae::buffer_base::get(l,2,true);
		if (buf->get_len() != m_width*Image::get_format_bpp(m_format)) {
			l.argerror(2,"invalid row len %");
		}
		png_byte* row_pointer = const_cast<png_byte*>(reinterpret_cast<const png_byte*>(buf->get_base()));
	    png_write_rows(m_write, &row_pointer,1);
	}

	void PNGImage::write_data(lua::state& l) {
		auto buf = llae::buffer_base::get(l,2,true);
		if (!write_data_buffer(buf)) {
			l.argerror(2,"invalid buffer size %");
		}
	}
	bool PNGImage::write_data_buffer(const llae::buffer_base_ptr& buf) {
		if (buf->get_len() != m_width*m_height*Image::get_format_bpp(m_format)) {
			return false;
		}
		png_byte* row_pointer = const_cast<png_byte*>(reinterpret_cast<const png_byte*>(buf->get_base()));
		for (size_t i=0;i<m_height;++i) {
	    	png_write_rows(m_write, &row_pointer,1);
	    	row_pointer += m_width * Image::get_format_bpp(m_format);
	    }
	    return true;
	}

	llae::buffer_ptr PNGImage::end_write() {
		png_write_end(m_write, m_info);
		return std::move(m_png_data);
	}

	lua::multiret PNGImage::close(lua::state& l) {
		auto res = end_write();
		lua::push(l,std::move(res));
		return {1};
	}

	llae::buffer_ptr PNGImage::do_encode(const ImagePtr& img) {
		PNGImage encoder;
		if (!encoder.write_header(img->get_format(),img->get_width(),img->get_height())) {
			return {};
		}
		if (!encoder.write_data_buffer(img->get_data())) {
			return {};
		}
		return std::move(encoder.end_write());
	}

	struct ReadCtx {
	    llae::buffer_base_ptr data;
	    size_t pos;
	};

	static void png_read_fn(png_structp png, png_bytep data, size_t size) {
	    auto& ctx = *static_cast<ReadCtx*>(png_get_io_ptr(png));
	    assert(ctx.data);
	    if (ctx.pos >= ctx.data->get_len()) {
	        return;
	    }
	    auto tail = ctx.data->get_len() - ctx.pos;
	    if (size > tail) {
	        size = tail;
	    }
	    memcpy(data, static_cast<const uint8_t*>(ctx.data->get_base())+ctx.pos, size);
	    ctx.pos += size;
	}
	ImagePtr PNGImage::do_decode(const llae::buffer_base_ptr& data) {
	    if (!data) {
	        return {};
	    }
		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                             PNGImage::error_fn,
	                                             PNGImage::warning_fn);
	    ReadCtx ctx{data,size_t(0)};
	    png_set_read_fn(png, &ctx, &png_read_fn);
	    
		png_infop info = png_create_info_struct(png);
		png_read_info(png, info);
		auto width = png_get_image_width(png, info);
	    auto height = png_get_image_height(png, info);
	    auto color_type = png_get_color_type(png, info);
	    auto bit_depth = png_get_bit_depth(png, info);
	    ImageFormat fmt = ImageFormat::RGBA;
	    if (bit_depth == 16)
	        png_set_strip_16(png);
	    if (color_type == PNG_COLOR_TYPE_RGB) {
	        fmt = ImageFormat::RGB;
	    }
	    if (color_type == PNG_COLOR_TYPE_PALETTE) {
	        png_set_palette_to_rgb(png);
	        fmt = ImageFormat::RGB;
	    }
	    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
	        png_set_expand_gray_1_2_4_to_8(png);
	        fmt = ImageFormat::GRAY;
	    }
	    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
	        png_set_tRNS_to_alpha(png);
	        fmt = ImageFormat::RGBA;
	    }
	    if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
	    	png_set_gray_to_rgb(png);
	    	fmt = ImageFormat::RGBA;
	    }
	    png_read_update_info(png, info);
	    ImagePtr res{new Image(width,height,fmt,{})};
	    auto row_bytes = png_get_rowbytes(png,info);
	    if ((res->get_width()*res->get_bpp()) != row_bytes) {
	    	return {};
	    }
	    std::vector<png_bytep> rows;
	    rows.resize(height);
	    auto dst = static_cast<png_bytep>(res->get_data()->get_base());
	    for (int y = 0; y < height; y++) {
	    	rows[y] = dst;
	    	dst += row_bytes;
	    }
	    png_read_image(png, rows.data());
	    png_destroy_info_struct(png, &info);
	    png_destroy_read_struct(&png, NULL, NULL);
	    ctx.data.reset();
	    return std::move(res);
	}


	class image_encode_png : public llae::work<llae::buffer_base_ptr> {
		ImagePtr m_image;
		llae::buffer_base_ptr m_result;
		virtual void do_work() override final {
			m_result = PNGImage::do_encode(m_image);
		}
		virtual llae::result<llae::buffer_base_ptr> after_work(llae::app& a) override final {
			if (!m_result) {
				return llae::string_error::create("failed encode");
			}
			return llae::result<llae::buffer_base_ptr>(std::move(m_result));
		}
	public:
		explicit image_encode_png( ImagePtr&& image) : m_image(std::move(image)) {}
	};


	llae::result_promise_ptr<llae::buffer_base_ptr> PNGImage::encode(llae::app& app,ImagePtr image) {
		if (!image) {
			return llae::make_result_promise_string_error<llae::buffer_base_ptr>("need image");
		}

		auto work = common::make_intrusive<image_encode_png>(std::move(image));
		return work->async_run(app);
	}

	llae::result_promise_ptr<llae::buffer_base_ptr> PNGImage::lencode(lua::state& l,ImagePtr image) {
		return encode(llae::app::get(l),std::move(image));
	}


	class image_decode_png : public llae::work<ImagePtr> {
		llae::buffer_base_ptr m_data;
		ImagePtr m_result;
		virtual void do_work() override {
			m_result = PNGImage::do_decode(m_data);
		}
		virtual llae::result<ImagePtr> after_work(llae::app& a) override final {
			if (!m_result) {
				return llae::string_error::create("failed decode");
			}
			return llae::result<ImagePtr>(std::move(m_result));
		}
	public:
		explicit image_decode_png(llae::buffer_base_ptr&& data) : m_data(std::move(data)) {}
	};


	llae::result_promise_ptr<ImagePtr> PNGImage::decode(llae::app& app,llae::buffer_base_ptr data) {
		if (!data) {
			return llae::make_result_promise_string_error<ImagePtr>("need data");
		}
		auto work = common::make_intrusive<image_decode_png>(std::move(data));
		return work->async_run(app);
	}

	llae::result_promise_ptr<ImagePtr> PNGImage::ldecode(lua::state& l,llae::buffer_base_ptr data) {
		return decode(llae::app::get(l),std::move(data));
	}


}