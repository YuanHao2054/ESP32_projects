file(GLOB_RECURSE srcs *.c
                    custom/*.c
                    generated/*.c
                    generated/guider_customer_fonts/*.c
                    generated/guider_fonts/*.c
                    generated/images/*.c
                    )
 
set(include_dirs 
     
    custom 
    generated 
    generated/guider_customer_fonts
    generated/guider_fonts
    generated/images
    )
 
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    PRIV_REQUIRES main
    )
 