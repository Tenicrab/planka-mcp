package("workflow")
    add_urls("https://github.com/sogou/workflow.git")
    add_deps("openssl")

    on_install(function (package)
        local content = io.readfile("xmake.lua")
        if content then
            content = content:gsub('add_requires%("openssl"[^%)]*%)', 'add_requires("openssl", {system = true})')
            io.writefile("xmake.lua", content)
        end
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        import("package.tools.xmake").install(package, configs)
        os.cp("_include/workflow", package:installdir("include"))
    end)
package_end()
