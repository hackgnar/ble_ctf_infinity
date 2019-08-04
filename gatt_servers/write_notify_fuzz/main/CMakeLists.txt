set(COMPONENT_SRCS "write_notify_fuzz.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

if(GCC_NOT_5_2_0)
    component_compile_options(-Wno-unused-const-variable)
endif()
