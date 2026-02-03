cmake_minimum_required(VERSION 3.20)
project(AsistenteVirtec)

# Usamos el estándar 20 para aprovechar las funciones modernas
set(CMAKE_CXX_STANDARD 20)

# 1. Buscamos todas las librerías necesarias (una sola vez por cada una)
find_package(wxWidgets CONFIG REQUIRED COMPONENTS net core base)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 2. Creamos el ejecutable
add_executable(AsistenteVirtec main.cpp)

# 3. Vinculamos TODO lo que el proyecto necesita
target_link_libraries(AsistenteVirtec PRIVATE
        wx::net
        wx::core
        wx::base
        cpr::cpr
        nlohmann_json::nlohmann_json
)

# 4. Esto quita la consola negra de fondo en Windows (Solo para la versión final)
if(WIN32)
    set_target_properties(AsistenteVirtec PROPERTIES WIN32_EXECUTABLE TRUE)
endif()