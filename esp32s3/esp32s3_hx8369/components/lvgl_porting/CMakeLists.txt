file(GLOB_RECURSE SOURCES 	./*.c 
							)

idf_component_register(SRCS	${SOURCES}
                   INCLUDE_DIRS 
								.
                    REQUIRES    
                                esp_timer
                                lvgl
                                lvgl_esp32_drivers  
                              )
