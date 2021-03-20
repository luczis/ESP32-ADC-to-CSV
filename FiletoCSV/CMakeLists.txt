cmake_minimum_required (VERSION 3.10)
project (FiletoCSV)

# The main executable
add_executable(FiletoCSV
  main.cpp
)

set_property(TARGET FiletoCSV PROPERTY CXX_STANDARD 11)
target_compile_options(FiletoCSV PRIVATE -Wall)

