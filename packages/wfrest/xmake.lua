package("wfrest")
    set_homepage("https://github.com/wfrest/wfrest")
    add_urls("https://github.com/wfrest/wfrest.git")
    add_deps("workflow", "zlib", "openssl")

    on_install(function (package)
        local content = io.readfile("xmake.lua")
        if content then
            content = content:gsub('includes%s*%b()', 'includes("src")')
            content = content:gsub('add_requires%("workflow"[%s%a%=%,]*%)', 'add_requires("workflow", {configs = {shared = false}})')
            content = content:gsub('add_requires%("zlib"[%s%a%=%,]*%)', 'add_requires("zlib", {system = false, configs = {shared = false}})')
            io.writefile("xmake.lua", content)
        end

        local configs = {}
        local workflow_pkg = package:dep("workflow")
        if workflow_pkg then
            configs.cxflags = "-I" .. workflow_pkg:installdir("include")
            configs.ldflags = "-L" .. workflow_pkg:installdir("lib")
        end

        import("package.tools.xmake").install(package, configs)
        os.cp("src/(**)", package:installdir("include"))
    end)
package_end()
