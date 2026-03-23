#include "jpeg_image.h"
#include "lua/state.h"
#include "lua/stack.h"
#include "lua/bind.h"
#include "uv/work.h"
#include "uv/luv.h"

#include <iostream>
#include "image.h"

#include <vector>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <setjmp.h>

#include "llae-private/jpeglib.h"
#include "llae-private/jerror.h"

META_OBJECT_INFO(image::JPEGImage,meta::object)

namespace image {

// struct for handling jpeg errors
    struct lm_jpeg_error_mgr : jpeg_error_mgr
    {
        
        // for longjmp, to return to caller on a fatal error
        jmp_buf setjmp_buffer;
    };
    
    struct lm_jpeg_source_mgr : jpeg_source_mgr {
        
        llae::buffer_base_ptr    data;
        size_t data_pos;
        bool    start_file;
        
        static void lm_jpeg_init_source (j_decompress_ptr cinfo)
        {
            lm_jpeg_source_mgr * src = (lm_jpeg_source_mgr*)cinfo->src;
            src->start_file = true;
            src->bytes_in_buffer = 0;
            src->data_pos = 0;
        }
        
        
        
        static boolean lm_jpeg_fill_input_buffer (j_decompress_ptr cinfo)
        {
            lm_jpeg_source_mgr * src = (lm_jpeg_source_mgr*)cinfo->src;
            if (src->data_pos >= src->data->get_len() ) {
                return FALSE;
            }
            size_t len = src->data->get_len() - src->data_pos;
            src->bytes_in_buffer = len;
            src->next_input_byte = static_cast<const unsigned char*>(src->data->get_base()) + src->data_pos;
            src->data_pos += len;
            src->start_file = false;
            // DO NOTHING
            return TRUE;
        }
        
        static void lm_jpeg_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
        {
            lm_jpeg_source_mgr * src = (lm_jpeg_source_mgr*)cinfo->src;
            if (num_bytes>0) {
                src->next_input_byte += (size_t) num_bytes;
                src->data_pos += (size_t) num_bytes;

            }
        }
        
        static void lm_jpeg_term_source (j_decompress_ptr /*cinfo*/)
        {
            // DO NOTHING
        }

    };


    static void lm_jpeg_error_exit (j_common_ptr cinfo)
    {
        // unfortunately we need to use a goto rather than throwing an exception
        // as gcc crashes under linux crashes when using throw from within
        // extern c code
        
        // Always display the message
        (*cinfo->err->output_message) (cinfo);
        
        // cinfo->err really points to a irr_error_mgr struct
        lm_jpeg_error_mgr *myerr = (lm_jpeg_error_mgr*) cinfo->err;
        
        longjmp(myerr->setjmp_buffer, 1);
    }
    
    static void lm_jpeg_output_message(j_common_ptr cinfo)
    {
        // display the error message.
        char temp1[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, temp1);
        std::cout <<  temp1  << std::endl;
    }

    JPEGImage::JPEGImage() {}


    JPEGImage::~JPEGImage() {
    }

    ImagePtr JPEGImage::do_decode(const llae::buffer_base_ptr& data) {
        if (!data)
            return {};
       
        // allocate and initialize JPEG decompression object
        struct jpeg_decompress_struct cinfo;
        struct lm_jpeg_error_mgr jerr;
            
        //We have to set up the error handler first, in case the initialization
        //step fails.  (Unlikely, but it could happen if you are out of memory.)
        //This routine fills in the contents of struct jerr, and returns jerr's
        //address which we place into the link field in cinfo.
        
        cinfo.err = jpeg_std_error(&jerr);
        cinfo.err->error_exit = lm_jpeg_error_exit;
        cinfo.err->output_message = lm_jpeg_output_message;
            
        // compatibility fudge:
        // we need to use setjmp/longjmp for error handling as gcc-linux
        // crashes when throwing within external c code
        if (setjmp(jerr.setjmp_buffer))
        {
            // If we get here, the JPEG code has signaled an error.
            // We need to clean up the JPEG object and return.
            
            jpeg_destroy_decompress(&cinfo);
            
            // if the row pointer was created, we delete it.
            // return null pointer
            return {};
        }
            
        // Now we can initialize the JPEG decompression object.
        jpeg_create_decompress(&cinfo);
            
        // specify data source
        lm_jpeg_source_mgr jsrc;
        jsrc.data = data;
            
            
        jsrc.init_source = &lm_jpeg_source_mgr::lm_jpeg_init_source;
        jsrc.fill_input_buffer = &lm_jpeg_source_mgr::lm_jpeg_fill_input_buffer;
        jsrc.skip_input_data = &lm_jpeg_source_mgr::lm_jpeg_skip_input_data;
        jsrc.resync_to_restart = &jpeg_resync_to_restart;
        jsrc.term_source = &lm_jpeg_source_mgr::lm_jpeg_term_source;
        cinfo.src = &jsrc;
            
        // Decodes JPG input from whatever source
        // Does everything AFTER jpeg_create_decompress
        // and BEFORE jpeg_destroy_decompress
        // Caller is responsible for arranging these + setting up cinfo
        
        // read file parameters with jpeg_read_header()
        jpeg_read_header(&cinfo, TRUE);
            
        ImageFormat fmt = ImageFormat::RGB;
        cinfo.out_color_space=JCS_RGB;
        cinfo.out_color_components=3;
    	cinfo.do_fancy_upsampling=FALSE;
            
        // Start decompressor
        jpeg_start_decompress(&cinfo);
            
            
        // Get image data

        auto rowspan = cinfo.image_width * 3;
        auto width = cinfo.image_width;
        auto height = cinfo.image_height;
        

        ImagePtr res{new Image(width,height,fmt,{})};

        auto& output = res->get_data();

    	// Here we use the library's state variable cinfo.output_scanline as the
        // loop counter, so that we don't have to keep track ourselves.
        // Create array of row pointers for lib
        static std::vector<unsigned char*> rowPtr;
        rowPtr.resize(height);
            
        for( size_t i = 0; i < height; i++ )
            rowPtr[i] = &static_cast<unsigned char*>(output->get_base())[ i * rowspan ];
        
        size_t rowsRead = 0;
        
        while( cinfo.output_scanline < cinfo.output_height )
            rowsRead += jpeg_read_scanlines( &cinfo, &rowPtr[rowsRead], cinfo.output_height - rowsRead );
        
        // Finish decompression
        
        jpeg_finish_decompress(&cinfo);
        
        // Release JPEG decompression object
        // This is an important step since it will release a good deal of memory.
        jpeg_destroy_decompress(&cinfo);

        
        return std::move(res);
    }

    class image_decode_jpeg : public llae::work<ImagePtr> {
        llae::buffer_base_ptr m_data;
        ImagePtr m_result;
        virtual void do_work() override {
            m_result = JPEGImage::do_decode(m_data);
        }
        virtual llae::result<ImagePtr> after_work(llae::app& a) override final {
            if (!m_result) {
                return llae::string_error::create("failed decode");
            }
            return llae::result<ImagePtr>(std::move(m_result));
        }
    public:
        explicit image_decode_jpeg(llae::buffer_base_ptr&& data) : m_data(std::move(data)) {}
    };

    llae::result_promise_ptr<ImagePtr> JPEGImage::decode(llae::app& a,llae::buffer_base_ptr data) {
        if (!data) {
            return llae::make_result_promise_string_error<ImagePtr>("need data");
        }
        auto work = common::make_intrusive<image_decode_jpeg>(std::move(data));
        return work->async_run(a);
    }
    llae::result_promise_ptr<ImagePtr> JPEGImage::ldecode(lua::state& l,llae::buffer_base_ptr data) {
        return decode(llae::app::get(l),std::move(data));
    }


}
