package("maplog")
    set_homepage("https://github.com/Tenire/maplog")
    set_description("A simple and efficient logging library for C++")

    add_urls("https://github.com/Tenire/maplog.git")

    on_install(function (package)
        import("package.tools.xmake").install(package)
        -- 手动拷贝头文件，因为原仓库没配置 add_headerfiles
        os.cp("include/*.h", package:installdir("include"))
    end)

    on_test(function (package)
        assert(package:has_cxxfuncs("maplog::Logger::instance()", {includes = "logger.h"}))
    end)
package_end()
