include(FetchContent)

FetchContent_Declare(
  Cortex-M_Atomics
    GIT_REPOSITORY https://github.com/Javier-varez/cortex-m_atomics
    GIT_TAG main)

FetchContent_MakeAvailable(Cortex-M_Atomics)
