add_library(${CMAKE_PROJECT_NAME} OBJECT)
target_sources(${CMAKE_PROJECT_NAME}
  PUBLIC
  FILE_SET CXX_MODULES FILES
    loki23.cppm
    hierarchy.cppm
    multimethods.cppm
    type_list.cppm
)
