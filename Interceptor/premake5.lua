workspace "Interceptor"
    architecture "x64"

    configurations
    {
        "Debug",
        "Dist"
    }

project "Interceptor"
    kind "ConsoleApp"
    language "C++"
    targetdir ("bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
    objdir ("bin-int/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "src/posix",
        "src/win",
        "vendor/boost",
        "vendor/cryptopp870"
    }

    filter "system:windows"
        staticruntime "on"
        defines
        {
            "IN_PLATFORM_WINDOWS",
            "_WIN32_WINNT=0x0601"
        }
    filter { "system:windows", "action:vs*" }
        buildoptions
        {
            "/std:c++20"
        }
    filter { "system:windows", "action:gmake2" }
        buildoptions
        {
            "-std=c++2a"
        }
    filter { "system:windows", "configurations:Debug" }
        runtime "Debug"
    
    filter { "system:windows", "configurations:Dist" }
        runtime "Release"
    
    filter "system:linux"
        defines
        {
            "IN_PLATFORM_POSIX",
            "IN_OS_LINUX"
        }
        buildoptions
        {
            "-std=c++2a",
            "-pthread",
            "-L./vendor/libs",
            "-lcryptopp"
        }
    premake.tools.gcc.libraryDirectories.architecture.x86_64 = { "-pthread", "-L./vendor/libs", "-lcryptopp"}
    
    filter "system:macosx"
        defines
        {
            "IN_PLATFORM_POSIX",
            "IN_OS_MAC"
        }
        buildoptions
        {
            "-std=c++2a",
            "-arch x86_64"
        }
        linkoptions
        {
            "-arch x86_64",
            "-framework CoreFoundation",
            "-framework IOKit"
        }
    
    filter "configurations:Debug"
        defines
        {
            "IN_DEBUG_BUILD"
        }
        symbols "On"

    filter "configurations:Dist"
        defines
        {
            "IN_DIST_BUILD"
        }
        optimize "On"


        




-- IN_PLATFORM_WINDOWS; IN_PLATFORM_POSIX