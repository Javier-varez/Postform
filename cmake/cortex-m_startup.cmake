include(FetchContent)

FetchContent_Declare(
  Cortex-M_Startup
    GIT_REPOSITORY https://github.com/Javier-varez/cortex-m_startup
    GIT_TAG main)

FetchContent_MakeAvailable(Cortex-M_Startup)
