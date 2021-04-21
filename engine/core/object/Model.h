//
// (c) 2021 Eduardo Doria.
//

#ifndef MODEL_H
#define MODEL_H

#include "Mesh.h"
#include "buffer/ExternalBuffer.h"

#define MAX_EXTERNAL_BUFFERS 9

namespace tinygltf {class Model;}

namespace Supernova{
    class Model: public Mesh{
    private:
        static bool fileExists(const std::string &abs_filename, void *);
        static bool readWholeFile(std::vector<unsigned char> *out, std::string *err, const std::string &filepath, void *);
        static std::string readFileToString(const char* filename);

        bool loadGLTFBuffer(int bufferViewIndex, MeshComponent& mesh, int& eBufferIndex);
        bool loadGLTFTexture(int textureIndex, Texture& texture, std::string textureName);
        std::string getBufferName(int bufferViewIndex);

        tinygltf::Model* gltfModel;
        ExternalBuffer eBuffers[MAX_EXTERNAL_BUFFERS];
        std::vector<float> extraBuffer;

        //Not working property
        //void computeGLTFTangent(int accessorIndicesIndex, int accessorPosIndex, int accessorUVIndex);
        //std::vector<Vector3> tangentBuffer;

    public:
        Model(Scene* scene); 
        virtual ~Model();   

        bool loadOBJ(const char* filename);
        bool loadGLTF(const char* filename);
    };
}

#endif //MODEL_H