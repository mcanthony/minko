minko.project.library "minko-framework"

	-- fixme remove dependency with offscreen plugin
	if _OPTIONS['with-offscreen'] then
		minko.plugin.enable("offscreen")
	end

	configuration { }

	kind "StaticLib"

	language "C++"

	files {
		"src/**.hpp",
		"src/**.cpp",
		"include/**.hpp",
		"asset/**"
	}

	includedirs {
		"src"
	}
	
	-- json cpp
	files {
		"lib/jsoncpp/src/**.cpp",
		"lib/jsoncpp/src/**.hpp",
		"lib/jsoncpp/src/**.c",
		"lib/jsoncpp/src/**.h"
	}
	includedirs {
		"lib/jsoncpp/src"
	}
	defines {
		"JSON_IS_AMALGAMATION"
	}
	
	-- windows
	configuration { "windows32" }
		libdirs { "lib/glew/lib/windows32" }
		buildoptions {
			"/wd4503"
		}
		
	configuration { "windows64" }
		libdirs { "lib/glew/lib/windows64" }
