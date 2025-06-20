#include "image_compose.h"
#include <lua/state.h>
#include <lua/stack.h>
#include <lua/bind.h>
#include <uv/work.h>
#include <uv/luv.h>

META_OBJECT_INFO(ImageCompose,meta::object)



ImageCompose::ImageCompose() {

}
ImageCompose::~ImageCompose() {

}
void ImageCompose::lbind(lua::state& l) {
	lua::bind::function(l,"new",&ImageCompose::lnew);
	lua::bind::function(l,"draw",&ImageCompose::draw);
	lua::bind::function(l,"run",&ImageCompose::run);
	lua::bind::function(l,"get_width",&ImageCompose::get_width);
	lua::bind::function(l,"get_height",&ImageCompose::get_height);
}

int ImageCompose::get_width() const {
	if (!m_src) return 0;
	return m_src->get_width();
}

int ImageCompose::get_height() const {
	if (!m_src) return 0;
	return m_src->get_height();
}

lua::multiret ImageCompose::lnew(lua::state& l) {
	ImageComposePtr res(new ImageCompose());
	res->m_src = lua::stack<ImagePtr>::get(l,1);
	if (!res->m_src) {
		l.argerror(1,"image");
	}
	lua::push(l,std::move(res));
	return {1};
}
	
lua::multiret ImageCompose::draw(lua::state& l) {
	auto x = l.checkinteger(2);
	if (x<0) {
		l.argerror(2,"pos");
	}
	auto y = l.checkinteger(3);
	if (y<0) {
		l.argerror(3,"pos");
	}
	auto img = lua::stack<ImagePtr>::get(l,4);
	if (!img) {
		l.argerror(4,"image");
	}
	auto sx = l.optinteger(5,0);
	if (sx<0) {
		l.argerror(5,"img pos");
	}
	auto sy = l.optinteger(6,0);
	if (sy<0) {
		l.argerror(6,"img pos");
	}
	auto sw = l.optinteger(7,img->get_width());
	auto sh = l.optinteger(8,img->get_height());
	if ((sx+sw)>img->get_width()) {
		l.argerror(7,"img w");
	}
	if ((sy+sh)>img->get_height()) {
		l.argerror(8,"img h");
	}
	if ((x+sw)>m_src->get_width()) {
		l.argerror(2,"pos r");
	}
	if ((y+sh)>m_src->get_height()) {
		l.argerror(3,"pos b");
	}
	m_draw.emplace_back(DrawSubImage{size_t(x),size_t(y),size_t(sx),size_t(sy),size_t(sw),size_t(sh),std::move(img)});
	l.pushboolean(true);
	return {1};
}

class ImageCompose::ComposeWork : public uv::lua_cont_work {
	ImageComposePtr m_compose;
	ImagePtr m_result;
	virtual void on_work() override {
		m_result = m_compose->work();
	}
	virtual int resume_args(lua::state& l,int status) override {
		int args;
        if (status < 0) {
        	l.pushnil();
            uv::push_error(l,status);
            args = 2;
        } else {
            lua::push(l,std::move(m_result));
            args = 1;
        }
        return args;
	}
public:
	explicit ComposeWork(lua::ref&& cont,const ImageComposePtr& compose) :
		uv::lua_cont_work(std::move(cont)),m_compose(compose) {}
};

ImagePtr ImageCompose::work() {
	m_src = m_src->clone();
	for (auto& d:m_draw) {
		if (!m_src->draw(d.x,d.y,d.image->extract_spr(d.sx,d.sy,d.sw,d.sh))) {
			return {};
		}
	}
	return std::move(m_src);
}

lua::multiret ImageCompose::run(lua::state& l) {
	if (!l.isyieldable()) {
		l.argerror(1,"is async");
	}
	{
        l.pushthread();
        lua::ref cont;
        cont.set(l);
        common::intrusive_ptr<ComposeWork> work(new ComposeWork(std::move(cont),ImageComposePtr(this)));
        int r = work->queue_work(l);
        if (r < 0) {
            work->reset(l);
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
    }
    
    l.yield(0);
    return {0};
}
