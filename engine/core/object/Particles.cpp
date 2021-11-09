//
// (c) 2021 Eduardo Doria.
//

#include "Particles.h"
#include "math/Angle.h"

using namespace Supernova;

Particles::Particles(Scene* scene): Object(scene){
    addComponent<ParticlesComponent>({});

    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    particomp.buffer = &buffer;

	buffer.clearAll();
    buffer.addAttribute(AttributeType::POSITION, 3, 0);
    buffer.addAttribute(AttributeType::COLOR, 4, 3 * sizeof(float));
    buffer.addAttribute(AttributeType::POINTSIZE, 1, 7 * sizeof(float));
    buffer.addAttribute(AttributeType::POINTROTATION, 1, 8 * sizeof(float));
    buffer.addAttribute(AttributeType::TEXTURERECT, 4, 9 * sizeof(float));
    buffer.setStride(13 * sizeof(float));
    buffer.setRenderAttributes(true);
    buffer.setUsage(BufferUsage::STREAM);
}

Particles::~Particles(){

}

void Particles::updateParticlesSize(ParticlesComponent& particomp){
    particomp.needUpdate = true;
}

void Particles::addParticle(Vector3 position){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    particomp.particles.push_back({position, Vector4(1.0f, 1.0f, 1.0f, 1.0f), 30, 0, Rect(0, 0, 1, 1)});

    updateParticlesSize(particomp);
}

void Particles::addParticle(Vector3 position, Vector4 color){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    particomp.particles.push_back({position, color, 30, 0, Rect(0, 0, 1, 1)});

    updateParticlesSize(particomp);
}

void Particles::addParticle(Vector3 position, Vector4 color, float size, float rotation){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    particomp.particles.push_back({position, color, size, Angle::defaultToRad(rotation), Rect(0, 0, 1, 1)});

    updateParticlesSize(particomp);
}

void Particles::addParticle(Vector3 position, Vector4 color, float size, float rotation, Rect textureRect){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    particomp.particles.push_back({position, color, size, Angle::defaultToRad(rotation), textureRect});
    particomp.hasTextureRect = true;

    updateParticlesSize(particomp);
}

void Particles::addParticle(float x, float y, float z){
   addParticle(Vector3(x, y, z));
}

void Particles::setTexture(std::string path){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();

    particomp.texture.setPath(path);

    //TODO: update texture, reload entity
}

void Particles::addSpriteFrame(int id, std::string name, Rect rect){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    if (id >= 0 && id < MAX_SPRITE_FRAMES){
        particomp.framesRect[id] = {true, name, rect};
    }else{
        Log::Error("Cannot set frame id %s less than 0 or greater than %i", name.c_str(), MAX_SPRITE_FRAMES);
    }
}

void Particles::addSpriteFrame(std::string name, float x, float y, float width, float height){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();

    int id = 0;
    while ( (particomp.framesRect[id].active = true) && (id < MAX_SPRITE_FRAMES) ) {
        id++;
    }

    if (id < MAX_SPRITE_FRAMES){
        addSpriteFrame(id, name, Rect(x, y, width, height));
    }else{
        Log::Error("Cannot set frame %s. Sprite frames reached limit of %i", name.c_str(), MAX_SPRITE_FRAMES);
    }
}

void Particles::addSpriteFrame(float x, float y, float width, float height){
    addSpriteFrame("", x, y, width, height);
}

void Particles::addSpriteFrame(Rect rect){
    addSpriteFrame(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void Particles::removeSpriteFrame(int id){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();
    particomp.framesRect[id].active = false;
}

void Particles::removeSpriteFrame(std::string name){
    ParticlesComponent& particomp = getComponent<ParticlesComponent>();

    for (int id = 0; id < MAX_SPRITE_FRAMES; id++){
        if (particomp.framesRect[id].name == name){
            particomp.framesRect[id].active = false;
        }
    }
}