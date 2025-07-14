
--https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.1/libjpeg-turbo-3.0.1.tar.gz
--https://sourceforge.net/projects/libjpeg/files/libjpeg/6b/jpegsrc.v6b.tar.gz/download
--https://deac-ams.dl.sourceforge.net/project/libjpeg/libjpeg/6b/jpegsrc.v6b.tar.gz
--http://www.ijg.org/files/jpegsrc.v6b.tar.gz

name = 'libjpeg'
version = '8a'
archive = 'jpegsrc.v' .. version .. '.tar.gz'
url = 'http://www.ijg.org/files/' .. archive
hash = '5146e68be3633c597b0d14d3ed8fa2ea'
dir = name .. '-' .. version

function install()
	download(url,archive,hash)

	unpack_tgz(archive,dir,1)

	preprocess{
		src = dir .. '/jconfig.cfg',
		dst = 'build/include/jconfig.h',
		comment = {

		},
		replace_line = {
			['#undef HAVE_PROTOTYPES'] = '#define HAVE_PROTOTYPES',
			['#undef HAVE_UNSIGNED_CHAR'] = '#define HAVE_UNSIGNED_CHAR',
			['#undef HAVE_UNSIGNED_SHORT'] = '#define HAVE_UNSIGNED_SHORT',
			['#undef void'] = '',
			['#undef const'] = '',
			['#undef HAVE_STDDEF_H'] = '#define HAVE_STDDEF_H',
			['#undef HAVE_STDLIB_H'] = '#define HAVE_STDLIB_H',
		},
	
		insert_before = {
			['#ifdef JPEG_INTERNALS'] = [[

/* Define "boolean" as unsigned char, not int, per Windows custom */
#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */

]]
		}
	}
	move_files{
		['build/include/jpeglib.h'] = 		dir..'/jpeglib.h',
		['build/include/jerror.h'] = 		dir..'/jerror.h',
		['build/include/jmorecfg.h'] = 		dir..'/jmorecfg.h',
		['build/include/jpegint.h'] = 		dir..'/jpegint.h',
	}
end

dependencies = {
}

build_lib = {


	components = {
			'jaricom.c', 'jcapimin.c', 'jcapistd.c', 'jcarith.c', 'jccoefct.c','jccolor.c',
				'jcdctmgr.c', 'jchuff.c', 'jcinit.c', 'jcmainct.c', 'jcmarker.c', 'jcmaster.c', 'jcomapi.c', 'jcparam.c',
				'jcprepct.c', 'jcsample.c', 'jctrans.c', 'jdapimin.c', 'jdapistd.c', 'jdarith.c', 'jdcoefct.c',
				--'jdatadst.c', 
				'jdcolor.c', 'jddctmgr.c', 'jdhuff.c', 'jdinput.c', 'jdmainct.c', 'jdmarker.c', 'jdmaster.c', 'jdmerge.c',
				'jdpostct.c', 'jdsample.c', 'jdtrans.c', 'jerror.c', 'jfdctflt.c', 'jfdctfst.c', 'jfdctint.c', 'jidctflt.c',
				'jidctfst.c', 'jidctint.c', 'jmemmgr.c', 'jmemnobs.c', 'jquant1.c', 'jquant2.c', 'jutils.c',	'transupp.c' 
	},
	project = [[
		includedirs {
			'include'
		}
		files {
			<% for _,f in ipairs(lib.components) do %>
				<%= format_file(module.dir,f ) %>,<% end %>
		}
]]
}
