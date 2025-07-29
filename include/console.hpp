#pragma once
#include "vulkan_context.hpp"
#include "util.hpp"
#include "imgui.h"

#define IMGUI_COLOR_RED ImVec4(1.0f, 0.1f, 0.0f, 1.0f)
#define IMGUI_COLOR_GREEN ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define IMGUI_COLOR_BLUE ImVec4(0.0f, 0.7f, 1.0f, 1.0f)
#define IMGUI_COLOR_WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)



struct ConsoleText{
    ImVec4 color;
    std::string text;
};



class ConsoleInstance{

public:
	ConsoleInstance(vk_ctx& p_ctx);
	~ConsoleInstance();

    void output(const std::string& text, ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void output(const char* text, ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));


    void processCommand(const std::string& text);
    void processCommand(const char* text);


    vk_ctx& ctx;

	std::vector<ConsoleText> consoleBuffer;

	bool scrollOnOutput = false;
	bool stdOut = false;

    std::vector<std::string> splitString(const std::string& str, char delimiter);


};