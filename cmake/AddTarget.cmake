function(ADD_SHARED NAME SOURCE)
  add_library(${NAME} SHARED ${SOURCE})
  set_target_properties(${NAME} PROPERTIES
    DEBUG_POSTFIX "_d")
endfunction()

function(ADD_STATIC NAME SOURCE)
  add_library(${NAME} STATIC ${SOURCE})
  set_target_properties(${NAME} PROPERTIES
    DEBUG_POSTFIX "_d")
endfunction()

function(ADD_EXE NAME SOURCE)
  add_executable(${NAME} ${SOURCE})
  set_target_properties(${NAME} PROPERTIES
    DEBUG_POSTFIX "_d")
endfunction()
