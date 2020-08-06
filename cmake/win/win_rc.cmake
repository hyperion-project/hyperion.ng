# process a .rc file for windows
# Provides (BINARY_NAME)_WIN_RC_PATH with path to generated file
function(generate_win_rc_file BINARY_NAME)
	# target path to store generated files
	set(TARGET_PATH ${CMAKE_BINARY_DIR}/win_rc_file/${BINARY_NAME})
	# assets
	string(REPLACE "/" "\\\\" WIN_RC_ICON_PATH ${CMAKE_SOURCE_DIR}/cmake/nsis/installer.ico)
	# configure the rc file
	configure_file(
		${CMAKE_SOURCE_DIR}/cmake/win/win.rc.in
		${TARGET_PATH}/win.rc
	)
    # provide var for parent scope
  	set(${BINARY_NAME}_WIN_RC_PATH ${TARGET_PATH}/win.rc PARENT_SCOPE)
endfunction()
