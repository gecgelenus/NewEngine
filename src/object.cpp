#include "object.hpp"
#include "graphic_pipeline.hpp"

Object::~Object()
{
    for(ObjectPrimitive p: primitives){
        if(p.dataBuffer != nullptr){
            free(p.dataBuffer);
            p.dataBuffer = nullptr;
        }
    }
}


Object::Object(vk_ctx& p_ctx):
 ctx(p_ctx)
 {
    objectID = ctx.objectIDNext;
    ctx.objectIDNext++;
    ctx.objectIDMap.insert({objectID, this});
 };

void Object::formatData(GraphicPipeline* pipeline)
{
    for(int i = 0; i < primitives.size(); i++){
        primitives[i].stride = pipeline->strideSize;
        if(primitives[i].dataBuffer != nullptr){
            free(primitives[i].dataBuffer);
            primitives[i].dataBuffer = nullptr;
        }
        primitives[i].vc.resize(primitives[i].vertices.size() * (pipeline->strideSize/sizeof(float)));
        primitives[i].dataBuffer = primitives[i].vc.data();
        //primitives[i].dataBuffer = malloc(strideSize * primitives[i].vertices.size());
        primitives[i].dataSize = pipeline->strideSize * primitives[i].vertices.size();
        primitives[i].pipelineIndex = pipeline->id;

        //std::cout << "Primitive vertex count: " << primitives[i].vertices.size() << std::endl;
        serializePrimitive(pipeline->interfaceVariables, i, pipeline->strideSize);


    }

    
    
}

void Object::serializePrimitive(std::vector<SpvReflectInterfaceVariable *> inputVars, uint32_t index, uint32_t strideSize)
{
    uint32_t attribOffset = 0;
    for(int i = 0; i < inputVars.size(); i++){
        
        std::vector<float> defaultArr(10, 1.0f);
        void* srcBuffer = nullptr;

        bool defaultData = false;

        if(strcmp(inputVars[i]->name, "pos") == 0){
            if(primitives[index].vertices.size() > 0){
                srcBuffer =  primitives[index].vertices.data();
            }else{
                defaultData = true;
            }
        }else if(strcmp(inputVars[i]->name, "normal") == 0){

            if(primitives[index].normals.size() > 0){
                srcBuffer =  primitives[index].normals.data();

            }else{
                defaultData = true;
            }
        }else if(strcmp(inputVars[i]->name, "UV") == 0){

            if(primitives[index].UV.size() > 0){
                srcBuffer =  primitives[index].UV.data();
            }else{
                defaultData = true;
            }
        }else if(strcmp(inputVars[i]->name, "color") == 0){

            if(primitives[index].colors.size() > 0){
                srcBuffer =  primitives[index].colors.data();
            }else{
                defaultData = true;
            }
        }else if(strcmp(inputVars[i]->name, "modelIndex") == 0 || strcmp(inputVars[i]->name, "materialIndex") == 0){
            continue;
        }else{
            std::cout << "WARNING: Unknown name on vertex attribute inputs: " << inputVars[i]->name << std::endl;
            continue;
        }
        
        
        switch (inputVars[i]->format)
        {
        case VK_FORMAT_R32G32_SFLOAT:
            if(defaultData){
                for(int p = 0; p < primitives[index].vertices.size(); p++){
                    memcpy(primitives[index].dataBuffer + (p * strideSize) + (attribOffset), defaultArr.data(), sizeof(glm::vec2));
                }
                attribOffset += sizeof(glm::vec2);

                break;
            }

            for(int p = 0; p < primitives[index].vertices.size(); p++){
                memcpy(primitives[index].dataBuffer + (p * strideSize) + (attribOffset), srcBuffer + (p * sizeof(glm::vec2)), sizeof(glm::vec2));
            }
            attribOffset += sizeof(glm::vec2);
            break;
        case VK_FORMAT_R32G32B32_SFLOAT:

            if(defaultData){
                for(int p = 0; p < primitives[index].vertices.size(); p++){
                    memcpy(primitives[index].dataBuffer + (p * strideSize) + (attribOffset), defaultArr.data(), sizeof(glm::vec3));
                }
                attribOffset += sizeof(glm::vec3);

                break;

            }


            for(int p = 0; p < primitives[index].vertices.size(); p++){
                memcpy(primitives[index].dataBuffer + (p * strideSize) + (attribOffset), srcBuffer+ (p * sizeof(glm::vec3)), sizeof(glm::vec3));
            }
            attribOffset += sizeof(glm::vec3);

            break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:

            if(defaultData){
                for(int p = 0; p < primitives[index].vertices.size(); p++){
                    memcpy(primitives[index].dataBuffer + (p * strideSize) + (attribOffset), defaultArr.data(), sizeof(glm::vec4));
                }
                attribOffset += sizeof(glm::vec4);

                break;

            }


            for(int p = 0; p < primitives[index].vertices.size(); p++){
                memcpy(primitives[index].dataBuffer + (p * strideSize) + (attribOffset), srcBuffer+ (p * sizeof(glm::vec4)), sizeof(glm::vec4));
            }
            attribOffset += sizeof(glm::vec4);
            
            break;
        default:
            std::cout << "WARNING: Unknown data type on vertex attribute inputs!" << std::endl;
            break;
        }
				
        




    }
}
