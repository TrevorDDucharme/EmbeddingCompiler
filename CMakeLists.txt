project(EmbeddingCompiler)
set(CMAKE_CXX_STANDARD 20)

add_executable(EmbeddingCompiler main.cpp)

find_package(LibArchive REQUIRED)
target_link_libraries(EmbeddingCompiler PRIVATE LibArchive::LibArchive)

find_package(zstd CONFIG REQUIRED)
target_link_libraries(EmbeddingCompiler PRIVATE zstd::libzstd)