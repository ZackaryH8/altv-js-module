
# Set this to false, when using a custom nodejs build for testing
set(__deps_check_enabled true)

# Default to downloading deps from dev branch
set(__deps_branch "dev")
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE __current_branch)
if(__current_branch STREQUAL "rc")
    set(__deps_branch "rc")
elseif(__current_branch STREQUAL "release")
    set(__deps_branch "release")
endif()

function(DownloadDeps basePath)
    set(__deps_release_lib "${basePath}/nodejs/lib/Release/libnode.lib")
    set(__deps_release_dll "${basePath}/nodejs/lib/Release/libnode.dll")
    set(__deps_debug_lib "${basePath}/nodejs/lib/Debug/libnode.lib")
    set(__deps_debug_dll "${basePath}/nodejs/lib/Debug/libnode.dll")
    set(__deps_linux_lib "${basePath}/nodejs/lib/libnode.so.102")
    set(__deps_update_file "${basePath}/nodejs/update.json")

    if(WIN32)
        set(__deps_os_path_name "x64_win32")
    elseif(UNIX)
        set(__deps_os_path_name "x64_linux")
    endif()
    set(__deps_url_base_path "https://cdn.altv.mp/deps/nodejs/${__deps_branch}/${__deps_os_path_name}")

    if(__deps_check_enabled)
        if(WIN32)
            file(DOWNLOAD "${__deps_url_base_path}/Release/update.json" ${__deps_update_file})
            file(READ ${__deps_update_file} __deps_release_update_json)
            string(JSON __deps_release_hashes GET ${__deps_release_update_json} hashList)
            file(REMOVE ${__deps_update_file})

            if(EXISTS ${__deps_release_lib})
                file(SHA1 ${__deps_release_lib} __deps_release_checksum)
            else()
                set(__deps_release_checksum 0)
            endif()
            string(JSON __deps_release_checksum_cdn GET ${__deps_release_hashes} libnode.lib)
            if(NOT ${__deps_release_checksum} STREQUAL ${__deps_release_checksum_cdn})
                message("Downloading release static library...")
                file(DOWNLOAD "${__deps_url_base_path}/Release/libnode.lib" ${__deps_release_lib})
            endif()

            if(EXISTS ${__deps_release_dll})
                file(SHA1 ${__deps_release_dll} __deps_release_dll_checksum)
            else()
                set(__deps_release_dll_checksum 0)
            endif()
            string(JSON __deps_release_dll_checksum_cdn GET ${__deps_release_hashes} libnode.dll)
            if(NOT ${__deps_release_dll_checksum} STREQUAL ${__deps_release_dll_checksum_cdn})
                message("Downloading release dynamic library...")
                file(DOWNLOAD "${__deps_url_base_path}/Release/libnode.dll" ${__deps_release_dll})
            endif()

            # Only download debug binary in Debug builds
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                file(DOWNLOAD "${__deps_url_base_path}/Debug/update.json" ${__deps_update_file})
                file(READ ${__deps_update_file} __deps_debug_update_json)
                string(JSON __deps_debug_hashes GET ${__deps_debug_update_json} hashList)
                file(REMOVE ${__deps_update_file})

                if(EXISTS ${__deps_debug_lib})
                    file(SHA1 ${__deps_debug_lib} __deps_debug_checksum)
                else()
                    set(__deps_debug_checksum 0)
                endif()
                string(JSON __deps_debug_checksum_cdn GET ${__deps_debug_hashes} libnode.lib)
                if(NOT ${__deps_debug_checksum} STREQUAL ${__deps_debug_checksum_cdn})
                    message("Downloading debug static library...")
                    file(DOWNLOAD "${__deps_url_base_path}/Debug/libnode.lib" ${__deps_debug_lib})
                endif()

                if(EXISTS ${__deps_debug_dll})
                    file(SHA1 ${__deps_debug_dll} __deps_debug_dll_checksum)
                else()
                    set(__deps_debug_dll_checksum 0)
                endif()
                string(JSON __deps_debug_dll_checksum_cdn GET ${__deps_debug_hashes} libnode.dll)
                if(NOT ${__deps_debug_dll_checksum} STREQUAL ${__deps_debug_dll_checksum_cdn})
                    message("Downloading debug dynamic library...")
                    file(DOWNLOAD "${__deps_url_base_path}/Debug/libnode.dll" ${__deps_debug_dll})
                endif()
            endif()
        elseif(UNIX)
            file(DOWNLOAD "${__deps_url_base_path}/update.json" ${__deps_update_file})
            file(READ ${__deps_update_file} __deps_linux_update_json)
            string(JSON __deps_linux_hashes GET ${__deps_linux_update_json} hashList)
            file(REMOVE ${__deps_update_file})

            if(EXISTS ${__deps_linux_lib})
                file(SHA1 ${__deps_linux_lib} __deps_linux_checksum)
            else()
                set(__deps_linux_checksum 0)
            endif()
            string(JSON __deps_linux_checksum_cdn GET ${__deps_linux_hashes} libnode.so.102)
            if(NOT ${__deps_linux_checksum} STREQUAL ${__deps_linux_checksum_cdn})
                message("Downloading binaries...")
                file(DOWNLOAD "${__deps_url_base_path}/libnode.so.102" ${__deps_linux_lib})
            endif()
        endif()
    endif()
endfunction()
