#include "vulkan_context.hpp"
#include "console.hpp"
#include <sstream>
#include "render_batch.hpp"
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
        if(words[1] == "batch"){
            std::string name = "";

            for(int i = 2; i < words.size(); i++){ // Get flags
                if(words[i] == "-n"){
                    name = words[i+1];
                    i++;
                    continue;
                }
            }
            if(name == ""){
                output("Creating render batch: Name is not given", IMGUI_COLOR_RED);
                return;
            }

            for(RenderBatch* rBatch: ctx.rQueue->batchList){
                if(rBatch->name == name){
                    output("Creating render batch: A render batch with same name already exists", IMGUI_COLOR_RED);
                    return;
                }
            }

            // Creating Process
            RenderBatch* r = new RenderBatch(ctx, name);
            GraphicPipeline* pipeline = new GraphicPipeline(ctx, "../shaders/bin/simple.vert.spv","../shaders/bin/simple.frag.spv",ctx.params);
            r->graphicPipeline = pipeline;
            ctx.rQueue->batchList.push_back(r);
            
            output("Creating render batch: Render batch created.", IMGUI_COLOR_GREEN);
            return;

        }
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
