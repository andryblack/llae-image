
#include <lua/bind.h>
#include <uv/buffer.h>
#include <uv/work.h>
#include <uv/luv.h>
#include "png_image.h"
#include "jpeg_image.h"
#include "image.h"
#include "image_compose.h"

META_OBJECT_INFO(Image,meta::object)

Image::Image(size_t w,size_t h,ImageFormat fmt,uv::buffer_ptr&& data) : m_width(w),m_height(h),m_format(fmt),m_data(std::move(data)) {
	if (!m_data) {
		m_data = uv::buffer::alloc(w*h*get_bpp());
	}
}
void Image::lbind(lua::state& l) {
	lua::bind::function(l,"new",&Image::lnew);
	lua::bind::function(l,"get_width",&Image::get_width);
	lua::bind::function(l,"get_height",&Image::get_height);
	lua::bind::function(l,"get_format",&Image::get_format);
	lua::bind::function(l,"get_data",&Image::get_data);
	lua::bind::function(l,"apply_alpha",&Image::apply_alpha);
	lua::bind::function(l,"premultiply_alpha",&Image::premultiply_alpha);
	lua::bind::function(l,"flip_v",&Image::flip_v);
	lua::bind::function(l,"fill",&Image::fill);
	lua::bind::function(l,"extract",&Image::extract);
	lua::bind::function(l,"clone",&Image::clone);
	lua::bind::function(l,"draw",&Image::draw);
	lua::bind::function(l,"blend",&Image::blend);
    lua::bind::function(l,"gray_to_rgba", &Image::gray_to_rgba);
    lua::bind::function(l,"find_bounds",&Image::find_bounds);
    lua::bind::function(l,"compare",&Image::compare);
    lua::bind::function(l,"downsample",&Image::downsample);
}
lua::multiret Image::lnew(lua::state& l) {
	auto w = l.checkinteger(1);
	auto h = l.checkinteger(2);
	auto fmt = static_cast<ImageFormat>(l.checkinteger(3));
	if (fmt != ImageFormat::GRAY && fmt != ImageFormat::RGB && fmt != ImageFormat::RGBA) {
		l.argerror(3,"invalid");
	}
	auto data = uv::buffer::get(l,4,true);
    if (data) {
        if (data->get_len() != (w*h*Image::get_bpp(fmt))) {
            l.argerror(4, "invalid size");
        }
    }
	ImagePtr res{new Image(w,h,fmt,std::move(data))};
	lua::push(l,std::move(res));
	return {1};
}


ImagePtr Image::clone() {
	uv::buffer_ptr data = uv::buffer::alloc(m_data->get_len());
	memcpy(data->get_base(),m_data->get_base(),m_data->get_len());
	return ImagePtr{new Image(get_width(),get_height(),get_format(),std::move(data))};
}

void Image::premultiply_alpha() {
	if (m_format != ImageFormat::RGBA)
		return;
	auto data = static_cast<uint8_t*>(m_data->get_base());
	for (size_t y=0;y<m_height;++y) {
		for (size_t x=0;x<m_width;++x) {
			uint8_t a = data[3];
			data[0] = (uint16_t(data[0]) * a) / 0xff;
			data[1] = (uint16_t(data[1]) * a) / 0xff;
			data[2] = (uint16_t(data[2]) * a) / 0xff;
			data += 4;
		}
	}
}

void Image::gray_to_rgba(size_t clr) {
    if (m_format != ImageFormat::GRAY)
        return;
    auto src = static_cast<const uint8_t*>(m_data->get_base());
    auto resbuf = uv::buffer::alloc(m_width*m_height*4);
    auto dst = static_cast<uint8_t*>(resbuf->get_base());
    for (size_t y=0;y<m_height;++y) {
        for (size_t x=0;x<m_width;++x) {
            uint8_t a = *src++;
            dst[0] = (uint16_t(clr&0xff) * a) / 0xff;
            dst[1] = (uint16_t((clr>>8)&0xff) * a) / 0xff;
            dst[2] = (uint16_t((clr>>16)&0xff) * a) / 0xff;
            dst[3] = a;
            dst += 4;
        }
    }
    m_format = ImageFormat::RGBA;
    m_data = std::move(resbuf);
}

static inline uint8_t blend_pm(uint8_t dst,uint8_t src,uint8_t ia,uint8_t a) {
	// dst = dst*(1-a) + src * a // np
	// dst = dst*(1-a) + src // pm
	uint16_t r = uint16_t(dst)*ia;
	r = r / 0xff + src;
	return r > 0xff ? 0xff : r;
}

static inline uint8_t blend_npm(uint8_t dst,uint8_t src,uint8_t ia,uint8_t a) {
	// dst = dst*(1-a) + src * a // np
	// dst = dst*(1-a) + src // pm
	uint16_t r = uint16_t(dst)*ia;
	uint16_t rs = uint16_t(src)*a;
	r = r / 0xff + rs/0xff;
	return r > 0xff ? 0xff : r;
}

template <typename blend_func_t>
bool Image::draw_impl(size_t x,size_t y,const ImagePtr& img,blend_func_t blend_func) {
	if (!img) return false;
	if (x>=m_width) return false;
	if (y>=m_height) return false;
	if ((x+img->get_width())>m_width) return false;
	if ((y+img->get_height())>m_height) return false;
    if (m_format != ImageFormat::RGB) {
        if (m_format != img->get_format()) {
            return false;
        }
    }
	auto dst = static_cast<uint8_t*>(m_data->get_base())+(x+y*m_width)*get_bpp();
	size_t dst_stride = m_width * get_bpp();
	auto src = static_cast<const uint8_t*>(img->get_data()->get_base());
	size_t src_stride = img->get_width() * img->get_bpp();
    if (m_format == img->get_format() &&
        (img->get_format() != ImageFormat::RGBA)) {
        for (size_t y=0;y<img->get_height();++y) {
            memcpy(dst,src,src_stride);
            src += src_stride;
            dst += dst_stride;
        }
    }
	else if (img->get_format() == ImageFormat::RGBA) {
		if (m_format == ImageFormat::RGBA) {
			for (size_t y=0;y<img->get_height();++y) {
				auto ldst = dst;
				for (size_t x=0;x<img->get_width();++x) {
					auto a = 0xff-src[3];

					ldst[0] = blend_func(ldst[0],src[0],a,src[3]);
					ldst[1] = blend_func(ldst[1],src[1],a,src[3]);
					ldst[2] = blend_func(ldst[2],src[2],a,src[3]);
					ldst[3] = blend_func(ldst[3],src[3],a,src[3]);

					src += 4;
					ldst += 4;
				}
				//src += src_stride;
				dst += dst_stride;
			}
		} else if (m_format == ImageFormat::RGB) {
			for (size_t y=0;y<img->get_height();++y) {
				auto ldst = dst;
				for (size_t x=0;x<img->get_width();++x) {
					auto a = 0xff-src[3];

					ldst[0] = blend_func(ldst[0],src[0],a,src[3]);
					ldst[1] = blend_func(ldst[1],src[1],a,src[3]);
					ldst[2] = blend_func(ldst[2],src[2],a,src[3]);

					src += 4;
					ldst += 3;
				}
				//src += src_stride;
				dst += dst_stride;
			}
		} else {
			return false;
		}
	}  else {
		return false;
	}
	return true;
}

bool Image::draw(size_t x,size_t y,const ImagePtr& img) {
	return draw_impl(x,y,img,blend_pm);
}
bool Image::blend(size_t x,size_t y,const ImagePtr& img) {
	return draw_impl(x,y,img,blend_npm);
}



void Image::flip_v() {
	auto w = get_width();
	auto h = get_height();
	auto bpp = get_bpp();
	auto resbuffer = uv::buffer::alloc(w*h*bpp);
	const uint8_t* src = static_cast<const uint8_t*>(m_data->get_base());
	uint8_t* dst = static_cast<uint8_t*>(resbuffer->get_base());
	resbuffer->set_len(w*h*bpp);
	for (size_t y=0;y<h;++y) {
		size_t dy = (h-y-1);
		memcpy(dst+dy*w*bpp,src,w*bpp);
		src+=w*bpp;
	}
	m_data = std::move(resbuffer);
}

lua::multiret Image::apply_alpha(lua::state& l) {
	auto alphaImg = lua::stack<ImagePtr>::get(l,2);
	if (!alphaImg) {
		l.argerror(2,"need image");
	}
	if (alphaImg->get_format() != ImageFormat::GRAY) {
		l.argerror(2,"need gray");
	}
	if (get_format() != ImageFormat::RGB) {
		l.argerror(1,"not rgb");
	}
	if (alphaImg->get_width() != get_width() ||
		alphaImg->get_height() != get_height() ) {
		l.argerror(2,"size different");
	}
	auto resbuffer = uv::buffer::alloc(get_width()*get_height()*4);
	const uint8_t* src = static_cast<const uint8_t*>(m_data->get_base());
	const uint8_t* src_a = static_cast<const uint8_t*>(alphaImg->get_data()->get_base());
	uint8_t* dst = static_cast<uint8_t*>(resbuffer->get_base());
	for (size_t i=0;i<get_width()*get_height();++i) {
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src_a++;
	}
	resbuffer->set_len(get_width()*get_height()*4);
	m_data = std::move(resbuffer);
	m_format = ImageFormat::RGBA;
	l.pushboolean(true);
	return {1};
}

void Image::fill(size_t fill) {
	auto w = get_width();
	auto h = get_height();
	uint8_t* dst = static_cast<uint8_t*>(m_data->get_base());
	if (m_format == ImageFormat::RGBA) {
		for (size_t i=0;i<w*h;++i) {
			*dst++ = fill;
			*dst++ = fill >> 8;
			*dst++ = fill >> 16;
			*dst++ = fill >> 24;
		}
	} else if (m_format == ImageFormat::RGB) {
		for (size_t i=0;i<w*h;++i) {
			*dst++ = fill;
			*dst++ = fill >> 8;
			*dst++ = fill >> 16;
		}
	} else {
		for (size_t i=0;i<w*h;++i) {
			*dst++ = fill;
		}
	}
}

ImagePtr Image::extract_spr(size_t ex,size_t ey,size_t ew,size_t eh) {
	if ((ex == 0) && (ey == 0) && (ew == m_width) && (eh == m_height)) {
		return ImagePtr(this);
	}
	if ((ex+ew)>m_width) {
		return {};
	}
	if ((ey+eh)>m_height) {
		return {};
	}
	auto bpp = get_bpp();
	auto resbuffer = uv::buffer::alloc(ew*eh*bpp);
	const uint8_t* src = static_cast<const uint8_t*>(m_data->get_base());
	src += (ey*m_width+ex)*bpp;
	uint8_t* dst = static_cast<uint8_t*>(resbuffer->get_base());
	for (size_t y=0;y<eh;++y) {
		memcpy(dst,src,ew*bpp);
		src+=m_width*bpp;
		dst+=ew*bpp;
	}
	resbuffer->set_len(ew*eh*bpp);
	return ImagePtr(new Image(ew,eh,m_format,std::move(resbuffer)));
}

lua::multiret Image::extract(lua::state& l) {
	auto ex = l.checkinteger(2);
	auto ey = l.checkinteger(3);
	auto ew = l.checkinteger(4);
	auto eh = l.checkinteger(5);
	
	if (ex < 0) {
		l.argerror(2,"invalid pos");
	}
	if (ey < 0) {
		l.argerror(3,"invalid pos");
	}
	if ((ex+ew)>get_width()) {
		l.argerror(4,"invalid pos");
	}
	if ((ey+eh)>get_height()) {
		l.argerror(5,"invalid pos");
	}
	auto res = extract_spr(ex,ey,ew,eh);
	if (res) {
		lua::push(l,std::move(res));
		return {1};
	}
	l.pushnil();
	l.pushstring("failed");
	return {2};
}

lua::multiret Image::find_bounds(lua::state& l) {
	int left = 0;
	int top = 0;
	int right = m_width;
	int bottom = m_height;
	if (m_format == ImageFormat::RGBA) {
		const uint8_t* src = static_cast<const uint8_t*>(m_data->get_base());
		for (size_t y=0;y<m_height;++y) {
			bool found = false;

			for (size_t x=0;x<m_width;++x) {
				if (src[(y*m_width+x)*4+3]) {
					found = true;
					break;
				}
			}
			if (found) {
				top = y;
				break;
			}
		}
		for (size_t x=0;x<m_width;++x) {
			bool found = false;

			for (size_t y=0;y<m_height;++y) {
				if (src[(y*m_width+x)*4+3]) {
					found = true;
					break;
				}
			}
			if (found) {
				left = x;
				break;
			}
		}
		for (size_t yy=0;yy<m_height;++yy) {
			bool found = false;
			size_t y = m_height-yy-1;

			for (size_t x=0;x<m_width;++x) {
				if (src[(y*m_width+x)*4+3]) {
					found = true;
					break;
				}
			}
			if (found) {
				bottom = y+1;
				break;
			} else if (y == top) {
				bottom = top;
				break;
			}
		}
		for (size_t xx=0;xx<m_width;++xx) {
			bool found = false;
			size_t x = m_width-xx-1;

			for (size_t y=0;y<m_height;++y) {
				if (src[(y*m_width+x)*4+3]) {
					found = true;
					break;
				}
			}
			if (found) {
				right = x + 1;
				break;
			} else if (x == left) {
				right = left;
				break;
			}
		}
	}
	l.pushinteger(left);
	l.pushinteger(top);
	l.pushinteger(right-left);
	l.pushinteger(bottom-top);
	return {4};
}

lua::multiret Image::compare(lua::state& l) {
	auto cmpimg = lua::stack<ImagePtr>::get(l,2);
	if (!cmpimg) {
		l.argerror(2,"need image");
	}
	if (cmpimg->get_format() != ImageFormat::RGB) {
		l.argerror(2,"need RGB");
	}
	if (cmpimg->get_width() != get_width() ||
		cmpimg->get_height() != get_height() ) {
		l.argerror(2,"different size");
	}
	const uint8_t* src1 = static_cast<const uint8_t*>(m_data->get_base());
	const uint8_t* src2 = static_cast<const uint8_t*>(cmpimg->get_data()->get_base());
	size_t pixels = get_width() * get_height();
	size_t diff = 0;
	auto add_diff = [&](const uint8_t* s1,const uint8_t* s2) {
		if (*s1 > *s2) {
			diff += (*s1 - *s2);
		} else {
			diff += (*s2 - *s1);
		}
	};
	for (size_t i=0;i<pixels;++i) {
		add_diff(src1++,src2++);
		add_diff(src1++,src2++);
		add_diff(src1++,src2++);
	}
	l.pushinteger(diff);
	return {1};
}

static inline uint8_t ds(uint8_t a,uint8_t b,uint8_t c,uint8_t d) {
	uint16_t r = a;
	r+=b;r+=c;r+=d;
	return r / 4;
}

ImagePtr Image::downsample() {
	auto w = get_width() / 2;
	auto h = get_height() / 2;
	ImagePtr res{new Image(w,h,m_format,uv::buffer_ptr())};
	auto dst = static_cast<uint8_t*>(res->get_data()->get_base());
	auto src = static_cast<const uint8_t*>(m_data->get_base());
	auto src2 = src + get_width() * get_bpp();
	if (m_format == ImageFormat::RGB) {

		for (size_t y=0;y<h;++y) {
			auto ssrc1 = src;
			auto ssrc2 = src2;
			for (size_t x=0;x<w;++x) {
				*dst++ = ds(ssrc1[0],ssrc1[3+0],ssrc2[0],ssrc2[3+0]);
				*dst++ = ds(ssrc1[1],ssrc1[3+1],ssrc2[1],ssrc2[3+1]);
				*dst++ = ds(ssrc1[2],ssrc1[3+2],ssrc2[2],ssrc2[3+2]);

				ssrc1+=3*2;
				ssrc2+=3*2;
			}
			src += get_width() * 3 * 2;
			src2 += get_width() * 3 * 2;
		}
	} else if (m_format == ImageFormat::RGBA) {

		for (size_t y=0;y<h;++y) {
			auto ssrc1 = src;
			auto ssrc2 = src2;
			for (size_t x=0;x<w;++x) {
				*dst++ = ds(ssrc1[0],ssrc1[4+0],ssrc2[0],ssrc2[4+0]);
				*dst++ = ds(ssrc1[1],ssrc1[4+1],ssrc2[1],ssrc2[4+1]);
				*dst++ = ds(ssrc1[2],ssrc1[4+2],ssrc2[2],ssrc2[4+2]);
				*dst++ = ds(ssrc1[3],ssrc1[4+3],ssrc2[3],ssrc2[4+3]);

				ssrc1+=4*2;
				ssrc2+=4*2;
			}
			src += get_width() * 4 * 2;
			src2 += get_width() * 4 * 2;
		}
	} else {
		for (size_t y=0;y<h;++y) {
			auto ssrc1 = src;
			auto ssrc2 = src2;
			for (size_t x=0;x<w;++x) {
				*dst++ = ds(ssrc1[0],ssrc1[1+0],ssrc2[0],ssrc2[1+0]);
				ssrc1+=1*2;
				ssrc2+=1*2;
			}
			src += get_width() * 1 * 2;
			src2 += get_width() * 1 * 2;
		}
	}
	return res;
}


int luaopen_image(lua_State* L) {
	lua::state l(L);
	lua::bind::object<PNGImage>::register_metatable(l,&PNGImage::lbind);
	lua::bind::object<JPEGImage>::register_metatable(l,&JPEGImage::lbind);
	lua::bind::object<Image>::register_metatable(l,&Image::lbind);
	lua::bind::object<ImageCompose>::register_metatable(l,&ImageCompose::lbind);
	l.createtable();
	lua::bind::object<PNGImage>::get_metatable(l);
	l.setfield(-2,"PNGImage");
	lua::bind::object<JPEGImage>::get_metatable(l);
	l.setfield(-2,"JPEGImage");
	lua::bind::object<Image>::get_metatable(l);
	l.setfield(-2,"Image");
	lua::bind::object<ImageCompose>::get_metatable(l);
	l.setfield(-2,"ImageCompose");
	lua::bind::function(l,"decode_png",&PNGImage::decode);
	lua::bind::function(l,"decode_jpeg",&JPEGImage::decode);
	lua::bind::function(l,"encode_png",&PNGImage::encode);
	lua::bind::function(l,"compose",&ImageCompose::lnew);
	l.createtable();
	lua::bind::value(l,"GRAY",ImageFormat::GRAY);
	lua::bind::value(l,"RGB",ImageFormat::RGB);
	lua::bind::value(l,"RGBA",ImageFormat::RGBA);
	l.setfield(-2,"ImageFormat");
	return 1;
}
