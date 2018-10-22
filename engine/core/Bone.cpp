//
// (c) 2018 Eduardo Doria.
//

#include "Bone.h"
#include "Model.h"
#include "Log.h"

using namespace Supernova;

Bone::Bone(): Object(){
    name = "";
    model = NULL;
}

Bone::~Bone(){

}

void Bone::moveToBind(){
    position = bindPosition;
    rotation = bindRotation;
    scale = bindScale;

    updateMatrix();
}

unsigned int Bone::getId() const {
    return id;
}

void Bone::setId(unsigned int id) {
    Bone::id = id;
}

const std::string &Bone::getName() const {
    return name;
}

void Bone::setName(const std::string &name) {
    Bone::name = name;
}

const Vector3 &Bone::getBindPosition() const {
    return bindPosition;
}

void Bone::setBindPosition(const Vector3 &bindPosition) {
    Bone::bindPosition = bindPosition;
}

const Quaternion &Bone::getBindRotation() const {
    return bindRotation;
}

void Bone::setBindRotation(const Quaternion &bindRotation) {
    Bone::bindRotation = bindRotation;
}

const Vector3 &Bone::getBindScale() const {
    return bindScale;
}

void Bone::setBindScale(const Vector3 &bindScale) {
    Bone::bindScale = bindScale;
}

const Matrix4 &Bone::getOffsetMatrix() const {
    return offsetMatrix;
}

void Bone::setOffsetMatrix(const Matrix4 &offsetMatrix) {
    Bone::offsetMatrix = offsetMatrix;
}

Model* Bone::getModel() const {
    return model;
}

void Bone::updateMatrix(){
    Object::updateMatrix();

    if (model) {
        Matrix4 skinning = model->getInverseDerivedTransform() * modelMatrix * offsetMatrix;

        model->updateBone(id, skinning);
    }
}
