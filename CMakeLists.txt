cmake_minimum_required( VERSION 3.13 )

# Find necessary packages
find_package( Boost  COMPONENTS system thread )

message( STATUS "Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}" )
message( STATUS "Boost_LIBRARIES=${Boost_LIBRARIES}" )

set (SOURCE_FILES pinger.cpp )
# Build executable
add_executable( pinger ${SOURCE_FILES})

include_directories(${Boost_INCLUDE_DIRS})


# Define the wrapper library that wraps our library
target_link_libraries( pinger ${Boost_LIBRARIES})

# don't prepend wrapper library name with lib
set_target_properties( pinger PROPERTIES PREFIX "" )

