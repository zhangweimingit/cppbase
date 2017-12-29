find_package(PythonInterp)

set(STYLE_FILTER)

set(STYLE_FILTER ${STYLE_FILTER}-legal/copyright,)

function(add_static_check_target TARGET_NAME SOURCES_LIST)
    if(NOT PYTHONINTERP_FOUND)
        return()
    endif()

    list(REMOVE_DUPLICATES SOURCES_LIST)
    list(SORT SOURCES_LIST)

    add_custom_target(${TARGET_NAME}
        COMMAND "${CMAKE_COMMAND}" -E chdir
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${PYTHON_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/style/cpplint.py"
            "--filter=${STYLE_FILTER}"
            "--counting=detailed"
            "--extensions=h,hpp,cpp"
            "--linelength=250"
            ${SOURCES_LIST}
        DEPENDS ${SOURCES_LIST}
        COMMENT "Linting ${TARGET_NAME}"
        VERBATIM)
endfunction()
