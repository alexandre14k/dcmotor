add_rules("mode.debug", "mode.release")

add_requires("gtk4")
add_requires("fmt", {system = true})

target("dcmotor")
    set_kind("binary")
    set_languages("c++17")

    add_includedirs("src")
    add_includedirs("src/debug")
    add_includedirs("src/gui")
    add_includedirs("src/motor")
    add_includedirs("src/noise")
    add_includedirs("src/plant")

    add_files("src/*.cpp", "src/**/*.cpp")
    add_packages("fmt", "gtk4")
