cmake_minimum_required(VERSION 3.8)

macro(PRINT_VAR var)
    message(STATUS "${var}=${${var}}")
endmacro()

function(GetCamelCase var value)
    string(SUBSTRING ${value} 0 1 first_letter)
    string(TOLOWER ${first_letter} first_letter)
    string(REGEX REPLACE "^.(.*)" "${first_letter}\\1" result "${value}")
    set(${var} ${result} PARENT_SCOPE)
endfunction()

function(GetCurrentDirectoryName dir_name)
    get_filename_component(list_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    string(REPLACE " " "_" list_name ${list_name})
    set("${dir_name}" "${list_name}" PARENT_SCOPE)
endfunction()


# Analogue for 'set' command which defines readonly variable.
#
# Usage:
# SetReadonly(FOO value)
macro(SetReadonly var)
  # Set the variable itself
  set("${var}" "${ARGN}")
  # Store the variable's value for restore it upon modifications.
  set("_${var}_readonly_val" "${ARGN}")
  # Register a watcher for a variable
  variable_watch("${var}" readonly_guard)
endmacro()

# Watcher for a variable which emulates readonly property.
macro(readonly_guard VAR access value current_list_file stack)
  if ("${access}" STREQUAL "MODIFIED_ACCESS")
    message(WARNING "Attempt to change readonly variable '${VAR}'!")
    # Restore a value of the variable to the initial one.
    set(${VAR} "${_${VAR}_readonly_val}")
  endif()
endmacro()