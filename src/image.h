#pragma once


#include <meta/object.h>
#include <common/intrusive_ptr.h>
#include <llae/buffer.h>


namespace lua {
	class state;
	class multiret;
}

enum class ImageFormat {
	GRAY,
	RGB,
	RGBA
};

class Image;
using ImagePtr = common::intrusive_ptr<Image>;

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
	static lua::multiret lnew(lua::state& l);
	static size_t get_bpp(ImageFormat fmt) {
		return fmt == ImageFormat::RGB ? 3 : (fmt == ImageFormat::GRAY ? 1 : 4);
	}
	size_t get_bpp() const { return get_bpp(m_format); }
	size_t get_width() const { return m_width; }
	size_t get_height() const { return m_height; }
	size_t get_stride() const { return get_width() * get_bpp(); }
	bool convert(ImageFormat fmt,size_t clr);
	const llae::buffer_ptr& get_data() const { return m_data; }
	ImageFormat get_format() const { return m_format; }
	lua::multiret apply_alpha(lua::state& l);
	void flip_v();
	void fill(size_t clr);
	ImagePtr extract_spr(size_t x,size_t y,size_t w,size_t h);
	lua::multiret extract(lua::state& l);
	lua::multiret find_bounds(lua::state& l);
	lua::multiret compare(lua::state& l);
	bool draw(size_t x,size_t y,const ImagePtr& img);
	bool blend(size_t x,size_t y,const ImagePtr& img);
	void premultiply_alpha();
    void gray_to_rgba(size_t clr);
	ImagePtr clone();
	ImagePtr downsample();

};

