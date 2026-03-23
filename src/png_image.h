#pragma once

#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "llae-private/png.h"
#include "image.h"
#include "llae/work.h"

namespace lua {
	class state;
	class multiret;
}

namespace image {

	class PNGImage;
	using PNGImagePtr = common::intrusive_ptr<PNGImage>;

	/// @luabind
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
		llae::buffer_ptr m_png_data;
		void write(png_bytep, size_t);
		bool write_header(ImageFormat fmt,size_t w,size_t h);
		bool write_data_buffer(const llae::buffer_base_ptr& data);
		llae::buffer_ptr end_write();
	public:
		PNGImage();
		~PNGImage();
		
		/// @luabind(name=new)
		static llae::result<PNGImagePtr> lnew(int w,int h,ImageFormat fmt);
		static llae::buffer_ptr do_encode(const ImagePtr& img);
		static ImagePtr do_decode(const llae::buffer_base_ptr& data);

		static llae::result_promise_ptr<llae::buffer_base_ptr> encode(llae::app& a, ImagePtr image);
		static llae::result_promise_ptr<ImagePtr> decode(llae::app& a,llae::buffer_base_ptr data);

		/// @luabind(name=encode,async=true)
		static llae::result_promise_ptr<llae::buffer_base_ptr> lencode(lua::state&l, ImagePtr image);
		/// @luabind(name=decode,async=true)
		static llae::result_promise_ptr<ImagePtr> ldecode(lua::state& l,llae::buffer_base_ptr data);

		/// @luabind
		void write_row(lua::state& l);
		/// @luabind
		void write_data(lua::state& l);
		/// @luabind
		lua::multiret close(lua::state& l);
	};

	/// @luabind(async=true)
	static llae::result_promise_ptr<ImagePtr> decode_png(lua::state& l,llae::buffer_base_ptr data) {
		return PNGImage::ldecode(l,data);
	}
	/// @luabind(async=true)
	static llae::result_promise_ptr<llae::buffer_base_ptr> encode_png(lua::state& l,ImagePtr image) {
		return PNGImage::lencode(l,image);
	}


	

}
