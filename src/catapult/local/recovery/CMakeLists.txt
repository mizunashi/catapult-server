cmake_minimum_required(VERSION 3.14)

include_directories(${PROJECT_SOURCE_DIR}/../../extensions)

catapult_library_target(catapult.local.recovery)
target_link_libraries(catapult.local.recovery catapult.local)
target_link_libraries(catapult.local.recovery catapult.local.server catapult.finalization)
