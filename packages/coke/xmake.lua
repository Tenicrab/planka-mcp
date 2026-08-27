package("coke")
    set_homepage("https://github.com/kedixa/coke")
    add_urls("https://github.com/kedixa/coke.git")
    add_deps("workflow", "openssl")

    on_install(function (package)
        io.writefile("xmake.lua", [[
add_rules("mode.debug", "mode.release")
add_requires("workflow", {configs = {shared = false}})
add_requires("openssl", {system = true})

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
        import("package.tools.xmake").install(package)
    end)
package_end()
