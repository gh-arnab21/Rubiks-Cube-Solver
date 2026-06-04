project (cube_display)

# All source files.
set (SRC
  "main.cpp"
  "Util/VectorHelper.cpp"
  "Util/GLException.cpp"
  "Model/Iterator.cpp"
  "Model/Orbiter.cpp"
  "Model/Material/Material.cpp"
  "Model/Material/Gold.cpp"
  "Model/Material/Chrome.cpp"
  "Model/Material/Brass.cpp"
  "Model/Material/Pearl.cpp"
  "Model/Material/BlackPlastic.cpp"
  "Model/Material/WhitePlastic.cpp"
  "Model/Material/GrayPlastic.cpp"
  "Model/World.cpp"
  "Model/WorldObject/Cube.cpp"
  "Model/WorldObject/Torus.cpp"
  "Model/WorldObject/Sphere.cpp"
  "Model/WorldObject/Triangle.cpp"
  "Model/WorldObject/WorldObject.cpp"
  "Model/MatrixStack.cpp"
  "Model/Camera.cpp"
  "Model/Light/Light.cpp"
  "Model/Light/DistanceLight.cpp"
  "Model/Light/PositionalLight.cpp"
  "Model/Rotator.cpp"
  "View/WorldWindow.cpp"
  "Controller/GL/Program/Shader.cpp"
  "Controller/GL/Program/Program.cpp"
  "Controller/GL/Program/LitProgram.cpp"
  "Controller/Command/Renderer.cpp"
  "Controller/Command/Command.cpp"
  "Controller/Command/Mover.cpp"
  "Controller/Command/ViewManager.cpp"
  "Controller/Command/Looker.cpp")

# Build the executable
add_executable(cube_display ${SRC})

# Linking
target_link_libraries(cube_display PRIVATE glfw glew glm)
target_include_directories(cube_display PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(cube_display PRIVATE /W3 /wd4244 /wd4267)
else()
    target_compile_options(cube_display PRIVATE -Wall -std=c++17 -Wno-strict-aliasing)
endif()

# We need OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(cube_display PRIVATE OpenGL::GL)
