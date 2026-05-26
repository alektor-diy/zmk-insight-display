if(CONFIG_ZMK_INSIGHT_DISPLAY)
  zephyr_include_directories(include)
  add_subdirectory(src)
endif()
