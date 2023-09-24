//
// (c) 2023 Eduardo Doria.
//

#include "PhysicsSystem.h"
#include "Scene.h"
#include "util/Angle.h"

#include "box2d.h"
#include "util/Box2DContactListener.h"
#include "util/Box2DContactFilter.h"


#include "Jolt/Jolt.h"

#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"


// Based on: https://github.com/jrouwe/JoltPhysics/blob/master/HelloWorld/HelloWorld.cpp
namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};
namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};



using namespace Supernova;



class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface{
private:
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];

public:
	BPLayerInterfaceImpl(){
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};


PhysicsSystem::PhysicsSystem(Scene* scene): SubSystem(scene){
	signature.set(scene->getComponentType<Body2DComponent>());

	this->scene = scene;

    this->pointsToMeterScale = 64.0;

    b2Vec2 gravity(0.0f, 10.0f);
    world2D = new b2World(gravity);
  
    contactListener2D = new Box2DContactListener(scene, this);
    contactFilter2D = new Box2DContactFilter(scene, this);

    world2D->SetContactListener(contactListener2D);
    world2D->SetContactFilter(contactFilter2D);


	const uint cMaxBodies = 1024;
	const uint cNumBodyMutexes = 0;
	const uint cMaxBodyPairs = 1024;
	const uint cMaxContactConstraints = 1024;

    broad_phase_layer_interface = new BPLayerInterfaceImpl();
	object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();
	object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();

	// Now we can create the actual physics system.
	world3D = new JPH::PhysicsSystem();
	world3D->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);
}

PhysicsSystem::~PhysicsSystem(){
    delete world2D;
    delete contactListener2D;
    delete contactFilter2D;

    delete world3D;
    delete broad_phase_layer_interface;
    delete object_vs_broadphase_layer_filter;
    delete object_vs_object_layer_filter;
}

float PhysicsSystem::getPointsToMeterScale() const{
    return pointsToMeterScale;
}

void PhysicsSystem::setPointsToMeterScale(float pointsToMeterScale){
    this->pointsToMeterScale = pointsToMeterScale;
}

void PhysicsSystem::updateBodyPosition(Signature signature, Entity entity, Body2DComponent& body, bool updateAnyway){
    if (signature.test(scene->getComponentType<Transform>())){
        Transform& transform = scene->getComponent<Transform>(entity);

        if (transform.needUpdate || updateAnyway){
            b2Vec2 bPosition(transform.position.x / pointsToMeterScale, transform.position.y / pointsToMeterScale);
            body.body->SetTransform(bPosition, Angle::defaultToRad(transform.rotation.getRoll()));
        }
    }
}

void PhysicsSystem::createBody2D(Entity entity){
    Signature signature = scene->getSignature(entity);

    if (!signature.test(scene->getComponentType<Body2DComponent>())){
        scene->addComponent<Body2DComponent>(entity, {});
        loadBody2D(entity);
    }
}

void PhysicsSystem::removeBody2D(Entity entity){
    Signature signature = scene->getSignature(entity);

    if (signature.test(scene->getComponentType<Body2DComponent>())){
        destroyBody2D(scene->getComponent<Body2DComponent>(entity));
        scene->removeComponent<Body2DComponent>(entity);
    }
}

int PhysicsSystem::createRectShape2D(Entity entity, float width, float height){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2PolygonShape shape;
            // same as shape.SetAsBox but using center on left corner
            shape.m_count = 4;
            shape.m_vertices[0].Set(0, 0);
            shape.m_vertices[1].Set( width / pointsToMeterScale, 0);
            shape.m_vertices[2].Set( width / pointsToMeterScale,  height / pointsToMeterScale);
            shape.m_vertices[3].Set(0,  height / pointsToMeterScale);
            shape.m_normals[0].Set(0.0f, -1.0f);
            shape.m_normals[1].Set(1.0f, 0.0f);
            shape.m_normals[2].Set(0.0f, 1.0f);
            shape.m_normals[3].Set(-1.0f, 0.0f);
            shape.m_centroid.SetZero();

            loadShape2D(*body, &shape, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createCenteredRectShape2D(Entity entity, float width, float height, Vector2 center, float angle){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2PolygonShape shape;
            float halfW = width / 2.0 / pointsToMeterScale;
            float halfH = height / 2.0 / pointsToMeterScale;
            shape.SetAsBox(halfW, halfH, b2Vec2(center.x / pointsToMeterScale, center.y / pointsToMeterScale), Angle::defaultToRad(angle));

            loadShape2D(*body, &shape, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createPolygonShape2D(Entity entity, std::vector<Vector2> vertices){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2PolygonShape polygon;
            std::vector<b2Vec2> b2vertices(vertices.size());
            for (int i = 0; i < vertices.size(); i++){
                b2vertices[i].Set(vertices[i].x / pointsToMeterScale, vertices[i].y / pointsToMeterScale);
            }
            polygon.Set(&b2vertices[0], (int)vertices.size());

            loadShape2D(*body, &polygon, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createCircleShape2D(Entity entity, Vector2 center, float radius){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2CircleShape circle;
            circle.m_p.Set(center.x / pointsToMeterScale, center.y / pointsToMeterScale);
            circle.m_radius = radius / pointsToMeterScale;

            loadShape2D(*body, &circle, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createTwoSidedEdgeShape2D(Entity entity, Vector2 vertice1, Vector2 vertice2){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2EdgeShape edge;
            b2Vec2 v1(vertice1.x / pointsToMeterScale, vertice1.y / pointsToMeterScale);
            b2Vec2 v2(vertice2.x / pointsToMeterScale, vertice2.y / pointsToMeterScale);
            edge.SetTwoSided(v1, v2);

            loadShape2D(*body, &edge, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createOneSidedEdgeShape2D(Entity entity, Vector2 vertice0, Vector2 vertice1, Vector2 vertice2, Vector2 vertice3){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2EdgeShape edge;
            b2Vec2 v0(vertice0.x / pointsToMeterScale, vertice0.y / pointsToMeterScale);
            b2Vec2 v1(vertice1.x / pointsToMeterScale, vertice1.y / pointsToMeterScale);
            b2Vec2 v2(vertice2.x / pointsToMeterScale, vertice2.y / pointsToMeterScale);
            b2Vec2 v3(vertice3.x / pointsToMeterScale, vertice3.y / pointsToMeterScale);
            edge.SetOneSided(v0, v1, v2, v3);

            loadShape2D(*body, &edge, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createLoopChainShape2D(Entity entity, std::vector<Vector2> vertices){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2ChainShape chain;
            std::vector<b2Vec2> b2vertices(vertices.size());
            for (int i = 0; i < vertices.size(); i++){
                b2vertices[i].Set(vertices[i].x / pointsToMeterScale, vertices[i].y / pointsToMeterScale);
            }
            chain.CreateLoop(&b2vertices[0], (int)vertices.size());

            loadShape2D(*body, &chain, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

int PhysicsSystem::createChainShape2D(Entity entity, std::vector<Vector2> vertices, Vector2 prevVertex, Vector2 nextVertex){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){

            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            b2ChainShape chain;
            std::vector<b2Vec2> b2vertices(vertices.size());
            for (int i = 0; i < vertices.size(); i++){
                b2vertices[i].Set(vertices[i].x / pointsToMeterScale, vertices[i].y / pointsToMeterScale);
            }
            b2Vec2 pv(prevVertex.x / pointsToMeterScale, prevVertex.y / pointsToMeterScale);
            b2Vec2 nv(nextVertex.x / pointsToMeterScale, nextVertex.y / pointsToMeterScale);
            chain.CreateChain(&b2vertices[0], (int)vertices.size(), pv, nv);

            loadShape2D(*body, &chain, body->numShapes);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

void PhysicsSystem::removeAllShapes(Entity entity){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        for (int i = 0; i < body->numShapes; i++){
            destroyShape2D(*body, i);
        }
        body->numShapes = 0;
    }
}

b2Body* PhysicsSystem::getBody(Entity entity){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        return body->body;
    }

    return NULL;
}

bool PhysicsSystem::loadBody2D(Entity entity){
    Body2DComponent& body = scene->getComponent<Body2DComponent>(entity);

    if (world2D && !body.body){
        b2BodyDef bodyDef;
        bodyDef.userData.pointer = entity;

        body.body = world2D->CreateBody(&bodyDef);
        body.newBody = true;

        return true;
    }

    return false;
}

void PhysicsSystem::destroyBody2D(Body2DComponent& body){
    // When a world leaves scope or is deleted by calling delete on a pointer
    // all the memory reserved for bodies, fixtures, and joints is freed
    if (world2D && body.body){
        for (int i = 0; i < body.numShapes; i++){
            destroyShape2D(body, i);
        }
        body.numShapes = 0;

        world2D->DestroyBody(body.body);

        body.body = NULL;
    }
}

bool PhysicsSystem::loadShape2D(Body2DComponent& body, b2Shape* shape, size_t index){
    if (world2D && !body.shapes[index].fixture){
        b2FixtureDef fixtureDef;
        fixtureDef.shape = shape;
        fixtureDef.userData.pointer = index;

        body.shapes[index].fixture = body.body->CreateFixture(&fixtureDef);

        return true;
    }

    return false;
}

void PhysicsSystem::destroyShape2D(Body2DComponent& body, size_t index){
    if (world2D && body.shapes[index].fixture){
        body.body->DestroyFixture(body.shapes[index].fixture);

        body.shapes[index].fixture = NULL;
    }
}

bool PhysicsSystem::loadDistanceJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchorA, Vector2 anchorB){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchorA(anchorA.x / pointsToMeterScale, anchorA.y / pointsToMeterScale);
        b2Vec2 myAnchorB(anchorB.x / pointsToMeterScale, anchorB.y / pointsToMeterScale);

        b2DistanceJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchorA, myAnchorB);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::DISTANCE;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadRevoluteJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchor){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchor(anchor.x / pointsToMeterScale, anchor.y / pointsToMeterScale);

        b2RevoluteJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchor);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::REVOLUTE;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadPrismaticJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchor, Vector2 axis){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchor(anchor.x / pointsToMeterScale, anchor.y / pointsToMeterScale);
        b2Vec2 myAxis(axis.x, axis.y);

        b2PrismaticJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchor, myAxis);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::PRISMATIC;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadPulleyJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 groundAnchorA, Vector2 groundAnchorB, Vector2 anchorA, Vector2 anchorB, Vector2 worldAxis, float ratio){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchorA(anchorA.x / pointsToMeterScale, anchorA.y / pointsToMeterScale);
        b2Vec2 myAnchorB(anchorB.x / pointsToMeterScale, anchorB.y / pointsToMeterScale);
        b2Vec2 myGroundA(groundAnchorA.x / pointsToMeterScale, groundAnchorA.y / pointsToMeterScale);
        b2Vec2 myGroundB(groundAnchorB.x / pointsToMeterScale, groundAnchorB.y / pointsToMeterScale);

        b2PulleyJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myGroundA, myGroundB, myAnchorA, myAnchorB, ratio);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::PULLEY;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadGearJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Entity revoluteJoint, Entity prismaticJoint, float ratio){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        Signature signatureJ1 = scene->getSignature(revoluteJoint);
        Signature signatureJ2 = scene->getSignature(prismaticJoint);

        if (signatureJ1.test(scene->getComponentType<Joint2DComponent>()) && signatureJ2.test(scene->getComponentType<Joint2DComponent>())){

            Joint2DComponent myRevoluteJoint = scene->getComponent<Joint2DComponent>(revoluteJoint);
            Joint2DComponent myPrismaticJoint = scene->getComponent<Joint2DComponent>(prismaticJoint);

            updateBodyPosition(signatureA, bodyA, myBodyA, true);
            updateBodyPosition(signatureB, bodyB, myBodyB, true);

            b2GearJointDef jointDef;
            jointDef.joint1 = myRevoluteJoint.joint;
            jointDef.joint2 = myPrismaticJoint.joint;
            jointDef.bodyA = myBodyA.body;
            jointDef.bodyB = myBodyB.body;
            jointDef.ratio = ratio;

            joint.joint = world2D->CreateJoint(&jointDef);
            joint.type = Joint2DType::GEAR;

        }else{
            Log::error("Cannot create joint, revoluteJoint or prismaticJoint not created");
            return false;
        }

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadMouseJoint2D(Joint2DComponent& joint, Entity body, Vector2 target){
    Signature signature = scene->getSignature(body);

    if (signature.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBody = scene->getComponent<Body2DComponent>(body);

        updateBodyPosition(signature, body, myBody, true);

        b2Vec2 myTarget(target.x / pointsToMeterScale, target.y / pointsToMeterScale);

        b2MouseJointDef jointDef;
        jointDef.bodyA = myBody.body;
        jointDef.bodyB = myBody.body;
        jointDef.target = myTarget;

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::MOUSE;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadWheelJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchor, Vector2 axis){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchor(anchor.x / pointsToMeterScale, anchor.y / pointsToMeterScale);
        b2Vec2 myAxis(axis.x, axis.y);

        b2WheelJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchor, myAxis);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::WHEEL;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadWeldJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchor){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchor(anchor.x / pointsToMeterScale, anchor.y / pointsToMeterScale);

        b2WeldJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchor);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::WELD;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadFrictionJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchor){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchor(anchor.x / pointsToMeterScale, anchor.y / pointsToMeterScale);

        b2FrictionJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchor);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::FRICTION;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadMotorJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2MotorJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body);

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::MOTOR;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

bool PhysicsSystem::loadRopeJoint2D(Joint2DComponent& joint, Entity bodyA, Entity bodyB, Vector2 anchorA, Vector2 anchorB){
    Signature signatureA = scene->getSignature(bodyA);
    Signature signatureB = scene->getSignature(bodyB);

    if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){

        Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(bodyA);
        Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(bodyB);

        updateBodyPosition(signatureA, bodyA, myBodyA, true);
        updateBodyPosition(signatureB, bodyB, myBodyB, true);

        b2Vec2 myAnchorA(anchorA.x / pointsToMeterScale, anchorA.y / pointsToMeterScale);
        b2Vec2 myAnchorB(anchorB.x / pointsToMeterScale, anchorB.y / pointsToMeterScale);

        b2DistanceJointDef jointDef;
        jointDef.Initialize(myBodyA.body, myBodyB.body, myAnchorA, myAnchorB);
        jointDef.minLength = 0;

        joint.joint = world2D->CreateJoint(&jointDef);
        joint.type = Joint2DType::ROPE;

    }else{
        Log::error("Cannot create joint, error in bodyA or bodyB");
        return false;
    }

    return true;
}

void PhysicsSystem::destroyJoint2D(Joint2DComponent& joint){
    if (world2D && joint.joint){
        world2D->DestroyJoint(joint.joint);

        joint.joint = NULL;
    }
}

void PhysicsSystem::load(){
}

void PhysicsSystem::destroy(){
    if (world2D){
        delete world2D;
        world2D = NULL;
    }
}

void PhysicsSystem::update(double dt){
	auto bodies2d = scene->getComponentArray<Body2DComponent>();
    auto joints2d = scene->getComponentArray<Joint2DComponent>();

	for (int i = 0; i < bodies2d->size(); i++){
		Body2DComponent& body = bodies2d->getComponentFromIndex(i);
		Entity entity = bodies2d->getEntity(i);
		Signature signature = scene->getSignature(entity);

        updateBodyPosition(signature, entity, body, body.newBody);

        body.newBody = false;
    }

    if (bodies2d->size() > 0){
        int32 velocityIterations = 6;
        int32 positionIterations = 2;

        world2D->Step(dt, velocityIterations, positionIterations);
    }

	for (int i = 0; i < bodies2d->size(); i++){
		Body2DComponent& body = bodies2d->getComponentFromIndex(i);
		Entity entity = bodies2d->getEntity(i);
		Signature signature = scene->getSignature(entity);

        b2Vec2 position = body.body->GetPosition();
        float angle = body.body->GetAngle();
        if (signature.test(scene->getComponentType<Transform>())){
		    Transform& transform = scene->getComponent<Transform>(entity);

            Vector3 nPosition = Vector3(position.x * pointsToMeterScale, position.y * pointsToMeterScale, transform.worldPosition.z);
            if (transform.position != nPosition){
                transform.position = nPosition;
                transform.needUpdate = true;
            }

            if (transform.worldRotation.getRoll() != angle){
                Quaternion rotation;
                rotation.fromAngle(Angle::radToDefault(angle));

                transform.rotation = rotation;
                transform.needUpdate = true;
            }

        }
    }
}

void PhysicsSystem::draw(){

}

void PhysicsSystem::entityDestroyed(Entity entity){
    Signature signature = scene->getSignature(entity);

    if (signature.test(scene->getComponentType<Body2DComponent>())){
        destroyBody2D(scene->getComponent<Body2DComponent>(entity));
    }
    if (signature.test(scene->getComponentType<Joint2DComponent>())){
        destroyJoint2D(scene->getComponent<Joint2DComponent>(entity));
    }
}
