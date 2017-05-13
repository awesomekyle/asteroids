 set(D3DX12_PATH ${CMAKE_SOURCE_DIR}/3rd-party/d3dx12)
 add_library(d3dx12 INTERFACE)
 target_include_directories(d3dx12
     INTERFACE
         ${D3DX12_PATH}
 )