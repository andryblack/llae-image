#pragma once

#include <meta/object.h>
#include <common/intrusive_ptr.h>
#include <png.h>
#include "image.h"

namespace lua {
	class state;
	class multiret;
}

class JPEGImage : public meta::object {
	META_OBJECT
private:
	
public:
	JPEGImage();
	~JPEGImage();
	static void lbind(lua::state& l);

	static ImagePtr do_decode(const llae::buffer_base_ptr& data);
	static lua::multiret decode(lua::state& l);
};
typedef common::intrusive_ptr<JPEGImage> JPEGImagePtr; 
