cmake_minimum_required(VERSION 3.19)
project(VisMol_V20 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

# Habilita el compilador automático de recursos de Qt (RCC)
set(CMAKE_AUTORCC ON)

# Compilación aislada y limpia exclusivamente orientada a la Interfaz Gráfica
qt_add_executable(VisMol_V20
    WIN32 MACOSX_BUNDLE
    main.cpp
    moleculavista.h
    moleculaview.h
    moleculalienzo.h
    moleculamat.h
    moleculavista.cpp
    moleculaview.cpp
    moleculalienzo.cpp
    moleculamat.cpp
    iconos.qrc            # Mantener el archivo aquí es correcto
)

target_link_libraries(VisMol_V20
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS VisMol_V20
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET VisMol_V20
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})