-- 利用包管理器，在下载 coke 后直接向其注入一个纯 xmake 的编译脚本，绕开 CMake，完美管理依赖
package("coke")
    add_urls("https://github.com/kedixa/coke.git")
    -- 指定下载 coke 需要的前置环境和底层依赖
    add_deps("workflow", "openssl")
    
    on_install(function (package)
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        -- 直接给下载的源码注入一个精简无外部依赖的 xmake.lua
        io.writefile("xmake.lua", [[
add_rules("mode.debug", "mode.release")

add_requires("workflow", {configs = {shared = false}})
add_requires("openssl")

target("coke")
    set_kind("static")
    set_languages("c++20")
    
    add_files("src/**.cpp")
    add_headerfiles("include/(**)")
    add_includedirs("include", {public = true})
    
    add_packages("workflow", "openssl")
    if is_plat("linux") then
        add_syslinks("pthread", "dl")
    end
        ]])

        import("package.tools.xmake").install(package, configs)
    end)
    
    on_test(function (package)
        assert(package:has_cxxfuncs("coke::sleep(0)", {includes = "coke/sleep.h", configs = {languages = "c++20"}}))
    end)
package_end()
