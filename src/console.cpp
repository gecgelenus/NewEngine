#include "vulkan_context.hpp"
#include "console.hpp"
#include <sstream>
#include "render_queue.hpp"


ConsoleInstance::ConsoleInstance(vk_ctx& p_ctx)
    : ctx(p_ctx)
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

void ConsoleInstance::processCommand(const std::string &text)
{
    std::vector<std::string> words = splitString(text, ' ');
    if(words[0] == "add"){ // Add command
        
    }
}

void ConsoleInstance::processCommand(const char *text)
{
    std::string t = text;
    processCommand(t);
}



std::vector<std::string> ConsoleInstance::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
