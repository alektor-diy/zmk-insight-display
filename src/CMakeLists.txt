target_sources(app PRIVATE
  state.c
  config.c
  events/state_changed.c
)

if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE
    central_state.c
    sync_service.c
  )
else()
  target_sources(app PRIVATE
    peripheral_sync.c
  )
endif()

if(CONFIG_ZMK_DISPLAY)
  target_sources(app PRIVATE
    display_cfb.c
    display_screen.c
  )
endif()
