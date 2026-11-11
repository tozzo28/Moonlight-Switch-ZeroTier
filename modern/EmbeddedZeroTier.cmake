# Shared transport from the working legacy build, linked into the original UI.
get_filename_component(ZT_INTEGRATION_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(ZT_SOURCE_ROOT "${ZT_INTEGRATION_ROOT}/src/zerotier")
add_library(moonlight_zerotier STATIC
    "${ZT_SOURCE_ROOT}/Embedded.cpp"
    "${ZT_SOURCE_ROOT}/StateStore.cpp"
    "${ZT_SOURCE_ROOT}/Transport.cpp"
    "${ZT_SOURCE_ROOT}/net/vnet.cpp"
    "${ZT_SOURCE_ROOT}/net/tcp_stack.cpp"
    "${ZT_SOURCE_ROOT}/net/lwip_unity.c")
target_compile_features(moonlight_zerotier PRIVATE cxx_std_17)
target_include_directories(moonlight_zerotier PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/app/zerotier-compat"
    "${ZT_SOURCE_ROOT}"
    "${ZT_SOURCE_ROOT}/include"
    "${ZT_SOURCE_ROOT}/lwip-port"
    "${ZT_SOURCE_ROOT}/lwip/src/include")
target_link_libraries(moonlight_zerotier PRIVATE borealis "${ZT_SOURCE_ROOT}/lib/libztcore.a")
target_include_directories(Moonlight PRIVATE "${ZT_SOURCE_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}/extern/glm")
target_link_libraries(Moonlight PRIVATE moonlight_zerotier)
foreach(fn socket connect bind close shutdown setsockopt getsockopt getsockname getpeername send recv sendto recvfrom sendmsg recvmsg poll select fcntl ioctl read write)
    target_link_options(Moonlight PRIVATE "-Wl,--wrap=${fn}")
endforeach()
# Use the curl archive rebuilt against the installed mbedTLS ABI.
target_link_libraries(Moonlight PRIVATE "${ZT_INTEGRATION_ROOT}/curl/lib/libcurl.a")
