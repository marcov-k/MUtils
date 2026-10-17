# CMakeList.txt : CMake project for CProjectBase, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.24)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (CProjectBase LANGUAGES C)

# Add source to this project's executable.
add_executable (CProjectBase  "main.c" "include/m_string.h" "src/m_string.c" "include/m_math_utils.h" "src/m_math_utils.c" "include/m_result.h" "include/m_contain.h" "include/m_vector.h" "src/m_vector.c" "include/m_deref.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CProjectBase PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
