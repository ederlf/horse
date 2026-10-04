find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
set(CYTHON_DEPENDENCIES
    ${PROJECT_SOURCE_DIR}/horse/__init__.pxd ${PROJECT_SOURCE_DIR}/horse/horse.pxd
    ${PROJECT_SOURCE_DIR}/horse/router.pxd)
foreach(module horse router)
    set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/cython/${module}.c")
    add_custom_command(
        OUTPUT "${GENERATED}"
        COMMENT "Generating the ${module} Cython extension"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/cython"
        COMMAND
            Python::Interpreter -m cython --3str --warning-errors -I
            "${PROJECT_SOURCE_DIR}" --output-file "${GENERATED}"
            "${PROJECT_SOURCE_DIR}/horse/${module}.pyx"
        DEPENDS "${PROJECT_SOURCE_DIR}/horse/${module}.pyx" ${CYTHON_DEPENDENCIES}
        VERBATIM)
    python_add_library(python_${module} MODULE "${GENERATED}" WITH_SOABI)
    target_link_libraries(python_${module} PRIVATE horse)
    set_target_properties(python_${module} PROPERTIES OUTPUT_NAME ${module}
                                                      INSTALL_RPATH "$ORIGIN")
    install(TARGETS python_${module} LIBRARY DESTINATION horse COMPONENT python)
endforeach()
install(TARGETS horse LIBRARY DESTINATION horse COMPONENT python)
