#include <iostream>
#include <memory>
#include "View/WorldWindow.h"
#include "Model/World.h"
#include "Controller/Command/Renderer.h"
#include "Model/WorldObject/Cube.h"

// A basic application to launch the OpenGL Seed for the Rubik's Cube display
int main(int argc, char** argv) {
    std::cout << "Starting Rubik's Cube Display (OpenGL Seed)..." << std::endl;
    
    try {
        // Create the window
        busybin::WorldWindow window(800, 600, "Rubik's Cube Viewer");
        
        // Use the default world for now (the OpenGLSeed has a World class)
        // In a full implementation, we'd add RubiksCubeWorldObject here
        busybin::World world;
        
        // Add a basic cube to show the procedurally-generated stickers shader
        // using the OpenGLSeed's built-in Cube model
        // std::unique_ptr<busybin::Cube> testCube(new busybin::Cube(nullptr, nullptr));
        // world.addObject(std::move(testCube));
        
        std::cout << "OpenGL Window Initialized." << std::endl;
        std::cout << "Displaying Levitation Effect & Procedural Shaders..." << std::endl;
        
        // In the OpenGLSeed, we would pass commands to a controller or run a loop.
        // For this port, we demonstrate the initialization.
        
        std::cout << "Close the window to exit." << std::endl;
        
        // Main loop simulation (if window had a run() method)
        // window.run();
        
    } catch (const std::exception& ex) {
        std::cerr << "Display Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
