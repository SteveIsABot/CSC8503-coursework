#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

StateGameObject::StateGameObject() {

	counter = 0.0f;

	distance = 0.0f;
	targetPos = Vector3();
	stateMachine = new StateMachine();

	State* Idle = new State([&](float dt)->void { distance = (targetPos - GetTransform().GetPosition()).Length(); });
	State* moveFwd = new State([&](float dt)->void { this->MoveForward(); });
	State* moveLft = new State([&](float dt)->void { this->MoveLeft(); });
	State* moveRgh = new State([&](float dt)->void { this->MoveRight(); });
	State* moveBkd = new State([&](float dt)->void { this->MoveBackwards(); });

	stateMachine->AddState(Idle);
	stateMachine->AddState(moveFwd);
	stateMachine->AddState(moveLft);
	stateMachine->AddState(moveRgh);
	stateMachine->AddState(moveBkd);

	stateMachine->AddTransition(new StateTransition(Idle, moveFwd, [&]()->bool{ return (targetPos - GetTransform().GetPosition()).z <= -3.5f; }));
	stateMachine->AddTransition(new StateTransition(moveFwd, Idle, [&]()->bool{ return (targetPos - GetTransform().GetPosition()).z > -3.5f; }));

	stateMachine->AddTransition(new StateTransition(Idle, moveBkd, [&]()->bool { return (targetPos - GetTransform().GetPosition()).z >= 3.5f; }));
	stateMachine->AddTransition(new StateTransition(moveBkd, Idle, [&]()->bool { return (targetPos - GetTransform().GetPosition()).z < 3.5f; }));

	stateMachine->AddTransition(new StateTransition(Idle, moveLft, [&]()->bool { return (targetPos - GetTransform().GetPosition()).x <= -3.5f; }));
	stateMachine->AddTransition(new StateTransition(moveLft, Idle, [&]()->bool { return (targetPos - GetTransform().GetPosition()).x > -3.5f; }));

	stateMachine->AddTransition(new StateTransition(Idle, moveRgh, [&]()->bool { return (targetPos - GetTransform().GetPosition()).x >= 3.5f; }));
	stateMachine->AddTransition(new StateTransition(moveRgh, Idle, [&]()->bool { return (targetPos - GetTransform().GetPosition()).x < 3.5f; }));

}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	stateMachine->Update(dt);
}

void StateGameObject::MoveLeft() {
	GetPhysicsObject()->AddForce({-10, 0, 0});
	distance = (targetPos - GetTransform().GetPosition()).Length();
}

void StateGameObject::MoveRight() {
	GetPhysicsObject()->AddForce({10, 0, 0 });
	distance = (targetPos - GetTransform().GetPosition()).Length();
}

void StateGameObject::MoveForward() {
	GetPhysicsObject()->AddForce({0, 0, -10});
	distance = (targetPos - GetTransform().GetPosition()).Length();
}

void StateGameObject::MoveBackwards() {
	GetPhysicsObject()->AddForce({0, 0, 10});
	distance = (targetPos - GetTransform().GetPosition()).Length();
}

void StateGameObject::CounterMotion() {
	Vector3 currentForce = GetPhysicsObject()->GetForce();
	GetPhysicsObject()->AddForce(-currentForce * 0.1f);
	distance = (targetPos - GetTransform().GetPosition()).Length();
}