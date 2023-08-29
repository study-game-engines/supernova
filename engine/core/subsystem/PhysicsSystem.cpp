//
// (c) 2023 Eduardo Doria.
//

#include "PhysicsSystem.h"
#include "Scene.h"
#include "util/Angle.h"

#include "box2d.h"


using namespace Supernova;

b2BodyType getBodyType(Body2DType type){
    if (type == Body2DType::STATIC){
        return b2_staticBody;
    }else if (type == Body2DType::kINEMATIC){
        return b2_kinematicBody;
    }else if (type == Body2DType::DYNAMIC){
        return b2_dynamicBody;
    }

    return b2_staticBody;
}


PhysicsSystem::PhysicsSystem(Scene* scene): SubSystem(scene){
	signature.set(scene->getComponentType<Body2DComponent>());

	this->scene = scene;

    this->world2D = NULL;
    this->pointsToMeterScale = 64.0;
}

PhysicsSystem::~PhysicsSystem(){
    if (world2D){
        delete world2D;
        world2D = NULL;
    }
}

void PhysicsSystem::createBody2D(Entity entity){
    Signature signature = scene->getSignature(entity);

    if (!signature.test(scene->getComponentType<Body2DComponent>())){
        scene->addComponent<Body2DComponent>(entity, {});
    }
}

void PhysicsSystem::removeBody2D(Entity entity){
    Signature signature = scene->getSignature(entity);

    if (signature.test(scene->getComponentType<Body2DComponent>())){
        destroyBody2D(scene->getComponent<Body2DComponent>(entity));
        scene->removeComponent<Body2DComponent>(entity);
    }
}

int PhysicsSystem::addRectShape2D(Entity entity, float width, float height){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        if (body->numShapes < MAX_SHAPES){
            body->shapes[body->numShapes].shape = new b2PolygonShape();
            body->shapes[body->numShapes].type = CollisionShape2DType::POLYGON;

            ((b2PolygonShape*)body->shapes[body->numShapes].shape)->SetAsBox(width/pointsToMeterScale, height/pointsToMeterScale);

            body->numShapes++;

            return (body->numShapes - 1);
        }else{
            Log::error("Cannot add more shapes in this body, please increase value MAX_SHAPES");
        }
    }

    return -1;
}

b2Body* PhysicsSystem::getBody(Entity entity){
    Body2DComponent* body = scene->findComponent<Body2DComponent>(entity);

    if (body){
        return body->body;
    }

    return NULL;
}

bool PhysicsSystem::loadBody2D(Body2DComponent& body){
    if (world2D && !body.body){
        b2BodyDef bodyDef;
        bodyDef.position.Set(0.0f, 0.0f);
        bodyDef.angle = 0.0f;
        bodyDef.linearVelocity = b2Vec2(body.linearVelocity.x, body.linearVelocity.y);
        bodyDef.angularVelocity = body.angularVelocity;
        bodyDef.linearDamping = body.linearDamping;
        bodyDef.angularDamping = body.angularDamping;
        bodyDef.allowSleep = body.allowSleep;
        bodyDef.awake = body.awake;
        bodyDef.fixedRotation = body.fixedRotation;
        bodyDef.bullet = body.bullet;
        bodyDef.enabled = body.enabled;
        bodyDef.gravityScale = body.gravityScale;
        bodyDef.type = getBodyType(body.type);

        body.needUpdate = false;

        body.body = world2D->CreateBody(&bodyDef);

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

        world2D->DestroyBody(body.body);

        body.body = NULL;
    }
}

bool PhysicsSystem::loadShape2D(Body2DComponent& body, size_t index){
    if (world2D && !body.shapes[index].fixture){
        b2FixtureDef fixtureDef;

        fixtureDef.density = body.shapes[index].density;
        fixtureDef.friction = body.shapes[index].friction;
        fixtureDef.restitution = body.shapes[index].restitution;
        fixtureDef.isSensor = body.shapes[index].sensor;
        fixtureDef.shape = body.shapes[index].shape;

        body.shapes[index].needUpdate = false;

        body.shapes[index].fixture = body.body->CreateFixture(&fixtureDef);

        return true;
    }

    return false;
}

void PhysicsSystem::destroyShape2D(Body2DComponent& body, size_t index){
    if (world2D && body.shapes[index].fixture){
        body.body->DestroyFixture(body.shapes[index].fixture);

        body.shapes[index].shape = NULL;
        body.shapes[index].fixture = NULL;
    }
}

bool PhysicsSystem::loadJoint2D(Joint2DComponent& joint){
    if (world2D && !joint.joint){
        Signature signatureA = scene->getSignature(joint.bodyA);
        Signature signatureB = scene->getSignature(joint.bodyB);

        if (signatureA.test(scene->getComponentType<Body2DComponent>()) && signatureB.test(scene->getComponentType<Body2DComponent>())){
            b2JointDef* jointDef = NULL;

            Body2DComponent myBodyA = scene->getComponent<Body2DComponent>(joint.bodyA);
            Body2DComponent myBodyB = scene->getComponent<Body2DComponent>(joint.bodyB);

            if (joint.type == Joint2DType::DISTANCE){
                b2Vec2 anchorA(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);
                b2Vec2 anchorB(joint.anchorB.x / pointsToMeterScale, joint.anchorB.y / pointsToMeterScale);

                jointDef = new b2DistanceJointDef();
                ((b2DistanceJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, anchorA, anchorB);
            }else if(joint.type == Joint2DType::REVOLUTE){
                b2Vec2 anchor(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);

                jointDef = new b2RevoluteJointDef();
                ((b2RevoluteJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, anchor);
            }else if(joint.type == Joint2DType::PRISMATIC){
                b2Vec2 anchor(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);
                b2Vec2 axis(joint.axis.x, joint.axis.y);

                jointDef = new b2PrismaticJointDef();
                ((b2PrismaticJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, anchor, axis);
            }else if(joint.type == Joint2DType::PULLEY){
                b2Vec2 anchorA(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);
                b2Vec2 anchorB(joint.anchorB.x / pointsToMeterScale, joint.anchorB.y / pointsToMeterScale);
                b2Vec2 groundA(joint.groundAnchorA.x / pointsToMeterScale, joint.groundAnchorA.y / pointsToMeterScale);
                b2Vec2 groundB(joint.groundAnchorB.x / pointsToMeterScale, joint.groundAnchorB.y / pointsToMeterScale);

                jointDef = new b2PulleyJointDef();
                ((b2PulleyJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, groundA, groundB, anchorA, anchorB, joint.ratio);
            }else if(joint.type == Joint2DType::GEAR){

                Signature signatureJ1 = scene->getSignature(joint.joint1);
                Signature signatureJ2 = scene->getSignature(joint.joint2);

                if (signatureJ1.test(scene->getComponentType<Joint2DComponent>()) && signatureJ2.test(scene->getComponentType<Joint2DComponent>())){
                    Joint2DComponent myJoint1 = scene->getComponent<Joint2DComponent>(joint.joint1);
                    Joint2DComponent myJoint2 = scene->getComponent<Joint2DComponent>(joint.joint2);

                    if (myJoint1.joint && myJoint2.joint){
                        jointDef = new b2GearJointDef();
                        ((b2GearJointDef*)jointDef)->joint1 = myJoint1.joint;
                        ((b2GearJointDef*)jointDef)->joint2 = myJoint2.joint;
                        ((b2GearJointDef*)jointDef)->bodyA = myBodyA.body;
                        ((b2GearJointDef*)jointDef)->bodyB = myBodyB.body;
                        ((b2GearJointDef*)jointDef)->ratio = joint.ratio;
                    }else{
                        Log::error("Cannot create joint, joint1 or joint2 not created");
                    }
                }else{
                    Log::error("Cannot create joint, error in joint1 or joint2");
                }
            }else if(joint.type == Joint2DType::MOUSE){
                b2Vec2 myTarget(joint.target.x / pointsToMeterScale, joint.target.y / pointsToMeterScale);

                jointDef = new b2MouseJointDef();
                ((b2MouseJointDef*)jointDef)->bodyA = myBodyA.body;
                ((b2MouseJointDef*)jointDef)->bodyB = myBodyB.body;
                ((b2MouseJointDef*)jointDef)->target = myTarget;
            }else if(joint.type == Joint2DType::WHEEL){
                b2Vec2 anchor(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);
                b2Vec2 axis(joint.axis.x, joint.axis.y);

                jointDef = new b2WheelJointDef();
                ((b2WheelJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, anchor, axis);
            }else if(joint.type == Joint2DType::WELD){
                b2Vec2 anchor(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);

                jointDef = new b2WeldJointDef();
                ((b2WeldJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, anchor);
            }else if(joint.type == Joint2DType::FRICTION){
                b2Vec2 anchor(joint.anchorA.x / pointsToMeterScale, joint.anchorA.y / pointsToMeterScale);

                jointDef = new b2FrictionJointDef();
                ((b2FrictionJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body, anchor);
            }else if(joint.type == Joint2DType::MOTOR){

                jointDef = new b2MotorJointDef();
                ((b2MotorJointDef*)jointDef)->Initialize(myBodyA.body, myBodyB.body);
            }

            if (jointDef){
                jointDef->collideConnected = joint.collideConnected;
                joint.joint = world2D->CreateJoint(jointDef);

                joint.needUpdate = false;

                delete jointDef;

                return true;
            }else{
                Log::error("Cannot create joint");
            }

        }else{
            Log::error("Cannot create joint, error in bodyA or bodyB");
        }
    }

    return false;
}

void PhysicsSystem::destroyJoint2D(Joint2DComponent& joint){
    if (world2D && joint.joint){
        world2D->DestroyJoint(joint.joint);

        joint.joint = NULL;
    }
}

void PhysicsSystem::load(){
    if (!world2D){
        b2Vec2 gravity(0.0f, 10.0f);
        world2D = new b2World(gravity);
    }
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

        bool isNewBody = loadBody2D(body);

        if (body.needUpdate){
            body.body->SetLinearVelocity(b2Vec2(body.linearVelocity.x, body.linearVelocity.y));
            body.body->SetAngularVelocity(body.angularVelocity);
            body.body->SetLinearDamping(body.linearDamping);
            body.body->SetAngularDamping(body.angularDamping);
            body.body->SetSleepingAllowed(body.allowSleep);
            body.body->SetAwake(body.awake);
            body.body->SetFixedRotation(body.fixedRotation);
            body.body->SetBullet(body.bullet);
            body.body->SetEnabled(body.enabled);
            body.body->SetGravityScale(body.gravityScale);
            body.body->SetType(getBodyType(body.type));

            body.needUpdate = false;
        }

        for (int i = 0; i < body.numShapes; i++){
            loadShape2D(body, i);

            if (body.shapes[i].needUpdate){
                float oldDensity = body.shapes[i].fixture->GetDensity();

                body.shapes[i].fixture->SetDensity(body.shapes[i].density);
                body.shapes[i].fixture->SetFriction(body.shapes[i].friction);
                body.shapes[i].fixture->SetRestitution(body.shapes[i].restitution);
                body.shapes[i].fixture->SetSensor(body.shapes[i].sensor);

                if (oldDensity != body.shapes[i].density){
                    body.body->ResetMassData();
                }

                body.shapes[i].needUpdate = false;
            }
        }

        if (signature.test(scene->getComponentType<Transform>())){
		    Transform& transform = scene->getComponent<Transform>(entity);

            if (isNewBody || transform.needUpdate){
                b2Vec2 bPosition(transform.worldPosition.x / pointsToMeterScale, transform.worldPosition.y / pointsToMeterScale);
                body.body->SetTransform(bPosition, transform.worldRotation.getRoll());
            }
        }
    }

	for (int i = 0; i < joints2d->size(); i++){
		Joint2DComponent& joint = joints2d->getComponentFromIndex(i);
		Entity entity = joints2d->getEntity(i);
		Signature signature = scene->getSignature(entity);

        if (joint.needUpdate){
            destroyJoint2D(joint);

            joint.needUpdate = false;
        }

        loadJoint2D(joint);
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

        body.linearVelocity = Vector2(body.body->GetLinearVelocity().x, body.body->GetLinearVelocity().y);
        body.angularVelocity = body.body->GetAngularVelocity();
        body.awake = body.body->IsAwake();
        body.enabled = body.body->IsEnabled();
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
