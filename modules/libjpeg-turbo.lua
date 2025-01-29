
--https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.1/libjpeg-turbo-3.0.1.tar.gz

name = 'libjpeg-turbo'
version = '3.0.1'
archive = 'libjpeg-turbo-' .. version .. '.tar.gz'
url = 'https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/'..version..'/' .. archive
hash = '1fdc6494521a8724f5f7cf39b0f6aff3'
dir = name .. '-' .. version

function install()
	download(url,archive,hash)

	unpack_tgz(archive)

	preprocess{
		src = dir .. '/jconfig.h.in',
		dst = 'build/include/jconfig.h',
		comment = {

		},
		replace_line = {
			['#define JPEG_LIB_VERSION  @JPEG_LIB_VERSION@'] = '#define JPEG_LIB_VERSION 62',
			['#define LIBJPEG_TURBO_VERSION  @VERSION@'] = '#define LIBJPEG_TURBO_VERSION 3.0.1',
			['#define LIBJPEG_TURBO_VERSION_NUMBER  @LIBJPEG_TURBO_VERSION_NUMBER@'] = '#define LIBJPEG_TURBO_VERSION_NUMBER  3000001',
			['#cmakedefine C_ARITH_CODING_SUPPORTED 1'] = '#define C_ARITH_CODING_SUPPORTED 1',
			['#cmakedefine D_ARITH_CODING_SUPPORTED 1'] = '#define D_ARITH_CODING_SUPPORTED 1',
			['#cmakedefine WITH_SIMD 1'] = '/* #define WITH_SIMD 0 */',
			['#cmakedefine RIGHT_SHIFT_IS_UNSIGNED 1'] = '/* #define RIGHT_SHIFT_IS_UNSIGNED 1 */',
		}
	}
	preprocess{
		src = dir .. '/jversion.h.in',
		dst = 'build/include/jversion.h',
	}
	preprocess{
		src = dir .. '/jconfigint.h.in',
		dst = 'build/modules/' .. name .. '/' .. dir .. '/jconfigint.h',
		replace = {
			['BUILD'] = '"20231220"',
			['INLINE'] = '',
			['THREAD_LOCAL'] = '',
			['PACKAGE_NAME'] = '"libjpeg-turbo"',
			['VERSION'] = '"' .. version .. '"', 
		},
		replace_line = {
			['#define SIZEOF_SIZE_T  @SIZE_T@'] = [[
#include <stddef.h>
#define SIZEOF_SIZE_T (SIZE_WIDTH/8)
			]],
			['#cmakedefine HAVE_BUILTIN_CTZL'] = '',
			['#cmakedefine HAVE_INTRIN_H'] = '',
			['#cmakedefine C_ARITH_CODING_SUPPORTED 1'] = '#define C_ARITH_CODING_SUPPORTED 1',
			['#cmakedefine D_ARITH_CODING_SUPPORTED 1'] = '#define D_ARITH_CODING_SUPPORTED 1',
			['#cmakedefine WITH_SIMD 1'] = '/* #define WITH_SIMD 0 */',
		}
	}
	move_files{
		['build/include/jpeglib.h'] = 		dir..'/jpeglib.h',
		['build/include/jerror.h'] = 		dir..'/jerror.h',
		['build/include/jmorecfg.h'] = 		dir..'/jmorecfg.h',
		['build/include/jpegint.h'] = 		dir..'/jpegint.h',
		--.h
		--['build/include/pngconf.h'] = 	dir..'/pngconf.h',
	}
end

dependencies = {
	--'zlib'
}

build_lib = {


	components = {
			'jaricom.c', 'jcomapi.c', 'jidctred.c', 'jdphuff.c', 
			 'jdapimin.c', 'jdapistd.c', 'jdarith.c', 'jdatadst.c', 'jdcoefct.c',
			'jdcolor.c', 'jddctmgr.c', 'jdhuff.c', 'jdinput.c', 'jdmainct.c', 'jdmarker.c', 'jdmaster.c', 'jdmerge.c',
			'jdpostct.c', 'jdsample.c', 'jdtrans.c', 'jerror.c', 'jfdctflt.c', 'jfdctfst.c', 'jfdctint.c', 'jidctflt.c',
			'jidctfst.c', 'jidctint.c', 'jmemmgr.c', 'jmemnobs.c', 'jquant1.c', 'jquant2.c', 'jutils.c',	'transupp.c' ,
	},
	project = [[
		includedirs {
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,f ) %>,<% end %>
		}
		--defines      { "Z_HAVE_UNISTD_H" , "PNG_ARM_NEON_OPT=0" }
]]
}
