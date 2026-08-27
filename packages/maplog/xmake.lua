package("maplog")
    set_homepage("https://github.com/Tenire/maplog")
    add_urls("https://github.com/Tenire/maplog.git")

    on_install(function (package)
        import("package.tools.xmake").install(package)
    end)
package_end()
