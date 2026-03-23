-- project llae-image
project 'llae-image'
-- @modules@
module 'llae'
module 'libpng'
module 'libjpeg'

bind_headers('src')

premake{
	project = [[

		files {
			<%= format_file('src','*.cpp')%>,
			<%= format_file('src','*.h')%>,
		}
	]]
}