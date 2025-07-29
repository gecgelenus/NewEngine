#include "console.hpp"

ConsoleInstance::ConsoleInstance()
{
}

ConsoleInstance::~ConsoleInstance()
{
}

void ConsoleInstance::output(const std::string &text, ImVec4 color)
{
    output(text.c_str(), color);
}

void ConsoleInstance::output(const char *text, ImVec4 color)
{
    ConsoleText t{};
    t.text = text;
    t.color = color;
    consoleBuffer.push_back(t);
}

