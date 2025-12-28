name = 'llae-image'

dependencies = {
	'llae',
	'libjpeg',
	'libpng'
}

cmodules = {
	'image'
}

function install() 

end

includedir = '${dir}/src'

build_lib = {

	project = [[
		includedirs{
			<%= format_mod_file(project:get_module('llae'),'src')%>,
			'include',
			'include/llae-private'
		}
		files {
			<%= format_file(module.dir,'src','*.cpp')  %>,
		}
	]]
}
