// GameEngine.cpp : Defines the entry point for the application.
//

#include "GameEngine.h"
#include "src/World.hpp"

int main(int argc, char** argv)
{
	World world{};
	world.initialise();
	world.run();
	return 0;
}
