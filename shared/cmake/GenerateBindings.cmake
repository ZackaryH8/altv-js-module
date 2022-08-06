# Generates the header files for the JavaScript bindings

function(make_includable input_file output_file)
    file(READ ${input_file} content)
    set(delim "for_c++_include")
    set(content "R\"${delim}(\n${content})${delim}\"")
    file(WRITE ${output_file} "${content}")
endfunction(make_includable)

function(GenerateBindings)
    file(GLOB_RECURSE SHARED_BINDING_FILES "${PROJECT_SOURCE_DIR}/../shared/bindings/*.js")
    foreach(file IN LISTS SHARED_BINDING_FILES)
        make_includable(${file} "${file}.gen")
    endforeach()
    file(GLOB_RECURSE CLIENT_BINDING_FILES "${PROJECT_SOURCE_DIR}/../client/src/bindings/*.js")
    foreach(file IN LISTS CLIENT_BINDING_FILES)
        make_includable(${file} "${file}.gen")
    endforeach()
    file(GLOB_RECURSE SERVER_BINDING_FILES "${PROJECT_SOURCE_DIR}/../server/src/bindings/*.js")
    foreach(file IN LISTS SERVER_BINDING_FILES)
        make_includable(${file} "${file}.gen")
    endforeach()
endfunction()
