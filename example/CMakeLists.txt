

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(threadpool1  threadpoolTest1.cpp ${SRC_THRPOOL})
target_link_libraries(threadpool1 pthread)

add_executable(threadpool2  threadpoolTest2.cpp ${SRC_THRPOOL})
target_link_libraries(threadpool2 pthread)

add_executable(threadpool3  threadpoolTest3.cpp ${SRC_THRPOOL})
target_link_libraries(threadpool3 pthread)

add_executable(threadpool4  threadpoolTest4.cpp ${SRC_THRPOOL})
target_link_libraries(threadpool4 pthread)


