set(PLUGINS_FOLDERS "klvparse")

foreach(FOLDER ${PLUGINS_FOLDERS})
   add_subdirectory("${FOLDER}")
endforeach()
