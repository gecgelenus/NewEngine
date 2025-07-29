#pragma once

#include "util.hpp"
#include "imgui.h"


#define IMGUI_COLOR_RED ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define IMGUI_COLOR_GREEN ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define IMGUI_COLOR_BLUE ImVec4(0.0f, 0.7f, 1.0f, 1.0f)
#define IMGUI_COLOR_WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)



struct ConsoleText{
    ImVec4 color;
    std::string text;
};



class ConsoleInstance{

public:
	ConsoleInstance();
	~ConsoleInstance();

    void output(const std::string& text, ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void output(const char* text, ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

	

	std::vector<ConsoleText> consoleBuffer;

	bool scrollOnOutput = false;
	bool stdOut = false;


};