#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "jinsoku.h"
#include "luaparser.h"

bool debug = false;

extern bool SIMPLIFIED;

int MAX_LOD = 0; // The maximum LOD level

bool INITIALIZED = false; // Have the simplified models been created?

int main(int argc, char *argv[])
{

	jsk::Jinsoku jinsoku;

	// parsing the lua config file
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_dofile(L, "D:/Documents/School/Denmark/2023/Thesis/Vulkan/Code/Jinsoku/src/config.lua"))
	{
		std::cout << "Something went wrong loading the config file" << std::endl;
		std::cout << lua_tostring(L, -1) << std::endl;
		// pop failed attempt at loading settings
		lua_pop(L, 1);
	}
	else
	{
		std::cout << "Loaded configuration file" << std::endl;
	}

	// model
	lua_getglobal(L, "model");
	jinsoku.setModel(lua_tostring(L, -1));

	// runtime
	lua_getglobal(L, "runtime");
	int runtime = lua_tointeger(L, -1);
	// output name

	// format input options
	lua_getglobal(L, "runtime");
	std::string fileName = lua_tostring(L, -1);

	// LOD variables
	lua_getglobal(L, "debug");
	debug = lua_toboolean(L, -1);

	lua_getglobal(L, "SIMPLIFIED");
	SIMPLIFIED = lua_toboolean(L, -1);

	lua_getglobal(L, "MAX_LOD");
	MAX_LOD = lua_tonumber(L, -1);

	// init shit
	jinsoku.initWindow();
	jinsoku.createContext();
	jinsoku.initVulkan();

	// load model
	jinsoku.loadModel();

	// setup renderer
	jinsoku.createRenderer();

	// main loop
	jinsoku.mainLoop(fileName, runtime);

	// clean up happens when main goes out of scope
	return (EXIT_SUCCESS);
}