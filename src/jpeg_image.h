#pragma once

#include "meta/object.h"
#include "common/intrusive_ptr.h"
#include "image.h"
#include "llae/work.h"

namespace lua {
	class state;
	class multiret;
}

namespace image {

	/// @luabind
	class JPEGImage : public meta::object {
		META_OBJECT
	private:
		
	public:
		JPEGImage();
		~JPEGImage();
		

		static ImagePtr do_decode(const llae::buffer_base_ptr& data);
		static llae::result_promise_ptr<ImagePtr> decode(llae::app& a,llae::buffer_base_ptr data);
		/// @luabind(name=decode,async=true)
		static llae::result_promise_ptr<ImagePtr> ldecode(lua::state& l,llae::buffer_base_ptr data);
	};
	using JPEGImagePtr = common::intrusive_ptr<JPEGImage>; 

	
	static llae::result_promise_ptr<ImagePtr> decode_jpeg(lua::state& l,llae::buffer_base_ptr data) {
		return JPEGImage::ldecode(l,data);
	}
}
