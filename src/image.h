#pragma once


#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "llae/buffer.h"


namespace lua {
	class state;
	class multiret;
}

namespace image {

	/// @luabind
	enum class ImageFormat {
		GRAY,
		RGB,
		RGBA
	};

	class Image;
	using ImagePtr = common::intrusive_ptr<Image>;

	/// @luabind
	class Image : public meta::object {
		META_OBJECT
	private:
		size_t m_width;
		size_t m_height;
		ImageFormat m_format;
		llae::buffer_ptr m_data;
		template <typename BlendFunc>
		bool draw_impl(size_t x,size_t y,const ImagePtr& img,BlendFunc);
	public:
		explicit Image(size_t w,size_t h,ImageFormat fmt,llae::buffer_ptr&& data);
		static void lbind(lua::state& l);
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
		/// @luabind
		static size_t get_format_bpp(ImageFormat fmt) {
			return fmt == ImageFormat::RGB ? 3 : (fmt == ImageFormat::GRAY ? 1 : 4);
		}
		/// @luabind
		size_t get_bpp() const { return get_format_bpp(m_format); }
		/// @luabind
		size_t get_width() const { return m_width; }
		/// @luabind
		size_t get_height() const { return m_height; }
		/// @luabind
		size_t get_stride() const { return get_width() * get_bpp(); }
		/// @luabind
		bool convert(ImageFormat fmt,size_t clr);
		/// @luabind
		const llae::buffer_ptr& get_data() const { return m_data; }
		/// @luabind
		ImageFormat get_format() const { return m_format; }
		/// @luabind
		lua::multiret apply_alpha(lua::state& l);
		/// @luabind
		void flip_v();
		/// @luabind
		void fill(size_t clr);
		/// @luabind
		ImagePtr extract_spr(size_t x,size_t y,size_t w,size_t h);
		/// @luabind
		lua::multiret extract(lua::state& l);
		/// @luabind
		lua::multiret find_bounds(lua::state& l);
		/// @luabind
		lua::multiret compare(lua::state& l);
		/// @luabind
		bool draw(size_t x,size_t y,const ImagePtr& img);
		/// @luabind
		bool blend(size_t x,size_t y,const ImagePtr& img);
		/// @luabind
		void premultiply_alpha();
		/// @luabind
	    void gray_to_rgba(size_t clr);
	    /// @luabind
		ImagePtr clone();
		/// @luabind
		ImagePtr downsample();

	};

}

