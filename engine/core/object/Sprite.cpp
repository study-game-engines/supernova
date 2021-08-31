#include "Sprite.h"

#include "math/Color.h"

using namespace Supernova;

Sprite::Sprite(Scene* scene): Object(scene){
    addComponent<SpriteComponent>({});

    SpriteComponent& spritecomp = scene->getComponent<SpriteComponent>(entity);
    spritecomp.buffer = &buffer;
    spritecomp.indices = &indices;

	buffer.clearAll();
	buffer.addAttribute(AttributeType::POSITION, 3);
	buffer.addAttribute(AttributeType::TEXCOORD1, 2);
    buffer.addAttribute(AttributeType::COLOR, 4);

    Attribute* attVertex = buffer.getAttribute(AttributeType::POSITION);

    buffer.addVector3(attVertex, Vector3(0, 0, 0));
    buffer.addVector3(attVertex, Vector3(spritecomp.width, 0, 0));
    buffer.addVector3(attVertex, Vector3(spritecomp.width,  spritecomp.height, 0));
    buffer.addVector3(attVertex, Vector3(0,  spritecomp.height, 0));

    Attribute* attTexcoord = buffer.getAttribute(AttributeType::TEXCOORD1);

    buffer.addVector2(attTexcoord, Vector2(0.01f, 0.01f));
    buffer.addVector2(attTexcoord, Vector2(0.99f, 0.01f));
    buffer.addVector2(attTexcoord, Vector2(0.99f, 0.99f));
    buffer.addVector2(attTexcoord, Vector2(0.01f, 0.99f));

    Attribute* attColor = buffer.getAttribute(AttributeType::COLOR);

    buffer.addVector4(attColor, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    buffer.addVector4(attColor, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    buffer.addVector4(attColor, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    buffer.addVector4(attColor, Vector4(1.0f, 1.0f, 1.0f, 1.0f));


    static const uint16_t indices_array[] = {
        0,  1,  2,
        0,  2,  3,
    };

    indices.setValues(
        0, indices.getAttribute(AttributeType::INDEX),
        6, (char*)&indices_array[0], sizeof(uint16_t));
}

void Sprite::setColor(Vector4 color){
    SpriteComponent& spritecomp = scene->getComponent<SpriteComponent>(entity);

    spritecomp.color = Color::sRGBToLinear(color);
}

void Sprite::setColor(float red, float green, float blue, float alpha){
    setColor(Vector4(red, green, blue, alpha));
}

Vector4 Sprite::getColor(){
    SpriteComponent& spritecomp = scene->getComponent<SpriteComponent>(entity);

    return Color::linearTosRGB(spritecomp.color);
}

void Sprite::setTexture(std::string path){
    SpriteComponent& spritecomp = scene->getComponent<SpriteComponent>(entity);

    spritecomp.texture.setPath(path);

    //TODO: update texture, reload entity
}