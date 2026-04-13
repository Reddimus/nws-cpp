add_executable(example_basic basic_usage.cpp)
target_link_libraries(example_basic PRIVATE nws)

add_executable(example_forecast get_forecast.cpp)
target_link_libraries(example_forecast PRIVATE nws)

add_executable(example_observations get_observations.cpp)
target_link_libraries(example_observations PRIVATE nws)

add_executable(example_alerts get_alerts.cpp)
target_link_libraries(example_alerts PRIVATE nws)

add_executable(example_monitor weather_monitor.cpp)
target_link_libraries(example_monitor PRIVATE nws)
