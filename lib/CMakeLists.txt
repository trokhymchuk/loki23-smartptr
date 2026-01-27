add_library(${CMAKE_PROJECT_NAME} OBJECT)
target_sources(${CMAKE_PROJECT_NAME}
  PUBLIC
  FILE_SET CXX_MODULES FILES
    loki23.cppm
    hierarchy.cppm
    multimethods.cppm
    smart_ptr/smart_ptr.cppm
    smart_ptr/checking_policy.cppm
    smart_ptr/conversion_policy.cppm
    smart_ptr/ownership_policy.cppm
    smart_ptr/storage_policy.cppm
    type_list.cppm
)
