#pragma once


#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include <vector>

#include "image.h"


class ImageCompose : public meta::object {
	META_OBJECT
private:
	ImagePtr	m_src;
	struct DrawSubImage {
		size_t x;
		size_t y;
		size_t sx;
		size_t sy;
		size_t sw;
		size_t sh;
		ImagePtr image;
	};
	std::vector<DrawSubImage> m_draw;
	class ComposeWork;
	ImagePtr work();
public:
	ImageCompose();
	~ImageCompose();
	static void lbind(lua::state& l);
	static lua::multiret lnew(lua::state& l);
	int get_width() const;
	int get_height() const;
	lua::multiret draw(lua::state& l);
	lua::multiret run(lua::state& l);
};
typedef common::intrusive_ptr<ImageCompose> ImageComposePtr; 