#include "PlaneTerrain.h"

#include "Log.h"
#include "render/ObjectRender.h"

using namespace Supernova;

PlaneTerrain::PlaneTerrain(): Mesh() {
    primitiveType = S_PRIMITIVE_TRIANGLES_STRIP;
}

PlaneTerrain::PlaneTerrain(float width, float depth): PlaneTerrain() {
    this->width = width;
    this->depth = depth;
}

PlaneTerrain::~PlaneTerrain() {
}

bool PlaneTerrain::load(){

    buffers.resize(1);
    buffers[0].clearAll();
    buffers[0].setName("vertices");
    buffers[0].addAttribute(S_VERTEXATTRIBUTE_VERTICES, 3);
    buffers[0].addAttribute(S_VERTEXATTRIBUTE_TEXTURECOORDS, 2);
    buffers[0].addAttribute(S_VERTEXATTRIBUTE_NORMALS, 3);

    AttributeData* attVertex = buffers[0].getAttribute(S_VERTEXATTRIBUTE_VERTICES);
    buffers[0].addValue(attVertex, Vector3(0, 0, 0));
    buffers[0].addValue(attVertex, Vector3(0, 0, depth));
    buffers[0].addValue(attVertex, Vector3(width, 0, 0));
    buffers[0].addValue(attVertex, Vector3(width, 0, depth));

    AttributeData* attTexcoord = buffers[0].getAttribute(S_VERTEXATTRIBUTE_TEXTURECOORDS);
    buffers[0].addValue(attTexcoord, Vector2(0.0f, 0.0f));
    buffers[0].addValue(attTexcoord, Vector2(0.0f, 1.0f));
    buffers[0].addValue(attTexcoord, Vector2(1.0f, 0.0f));
    buffers[0].addValue(attTexcoord, Vector2(1.0f, 1.0f));

    AttributeData* attNormal = buffers[0].getAttribute(S_VERTEXATTRIBUTE_NORMALS);
    buffers[0].addValue(attNormal, Vector3(0.0f, 1.0f, 0.0f));
    buffers[0].addValue(attNormal, Vector3(0.0f, 1.0f, 0.0f));
    buffers[0].addValue(attNormal, Vector3(0.0f, 1.0f, 0.0f));
    buffers[0].addValue(attNormal, Vector3(0.0f, 1.0f, 0.0f));

    return Mesh::load();
}
