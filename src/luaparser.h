#pragma once
#ifndef HEADERGUARD_LUAPARSER_H
#define HEADERGUARD_LUAPARSER_H

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <string>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

namespace jsk
{
	enum class PresentationMode
	{
		DESKTOP = 1,
		OPENVR = 2,
		OPENXR = 3,
		NONE = 4
	};

	enum class RenderMode
	{
		RASTERIZATION = 1,
		COMPUTE = 2,
		RAYTRACING = 3
	};

	struct Settings
	{
		bool stereoDisplay;
		uint32_t width = 800;
		uint32_t height = 600;
		VkExtent2D m_extent{width, height};
		std::vector<std::string> instanceExtensions;
		std::vector<const char *> optionalDeviceExtensions;
		std::vector<const char *> requiredDeviceExtensions;
		VkFormat colorFormat;
		VkFormat depthFormat;
		uint32_t numImageViews;
		VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		std::string model;
		std::string shader;
		std::string computeMode;
		int n, k, m;
	};

	struct Options
	{
		PresentationMode presentationMode;
		RenderMode renderMode;
	};

	static bool checkLua(lua_State *L, int r)
	{
		if (r != LUA_OK)
		{
			std::string errmsg = lua_tostring(L, -1);
			std::cout << errmsg << std::endl;
			return false;
		}

		return true;
	}

	static void clean(lua_State *L)
	{
		int n = lua_gettop(L);
		lua_pop(L, n);
	}

	static void loadOptions(lua_State *L, Options &options)
	{
		lua_getglobal(L, "engineMode");
		options.renderMode = (RenderMode)lua_tointeger(L, -1);
		// pop value
		clean(L);

		lua_getglobal(L, "presentationMode");
		options.presentationMode = (PresentationMode)lua_tointeger(L, -1);
		// pop value
		clean(L);
	}

	static void loadSettings(lua_State *L, Settings &settings)
	{
	}

	static void loadConfiguration(std::string configurationFile)
	{
		lua_State *L = luaL_newstate();
		luaL_openlibs(L);

		// Register exposed cpp functions
		// lua_register(L, "createRenderPass", jsvk::createRenderPass);

		std::string configFile = configurationFile + ".lua";
		if (luaL_dofile(L, configFile.c_str()))
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
	}
}
#endif
