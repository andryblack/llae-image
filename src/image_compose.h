#pragma once


#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "llae/work.h"
#include <vector>

#include "image.h"

namespace image {

	/// @luabind
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
		
		/// @luabind(name=new)
		static lua::multiret lnew(lua::state& l);
		/// @luabind
		int get_width() const;
		/// @luabind
		int get_height() const;
		/// @luabind
		lua::multiret draw(lua::state& l);
		/// @luabind(async=true)
		llae::result_promise_ptr<ImagePtr> run(lua::state& l);
	};

	using ImageComposePtr = common::intrusive_ptr<ImageCompose>;

	/// @luabind
	static lua::multiret compose(lua::state& l) {
		return ImageCompose::lnew(l);
	}

}
