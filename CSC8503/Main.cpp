#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>
#include <fstream>

void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void {
		std::cout << "I'm in State A!\n";
		data++;
	});
	
	State* B = new State([&](float dt)->void {
		std::cout << "I'm in State B!\n";
		data--;
	});

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool { return data > 10; });
	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool { return data < 0; });

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; i++) {
		testMachine->Update(1.0f);
	}

}

vector<Vector3> testNodes;
void TestPathfinding() {
	
	NavigationGrid grid("mazeGrid.txt");
	NavigationPath outPath;
	float unitLength = 15.0f;

	Vector3 startPos(unitLength * 1, 0, unitLength * 1);
	Vector3 endPos(unitLength * 5, 0, unitLength * 9);
	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) testNodes.push_back(pos);
}

void DisplayPathfinding() {

	for (int i = 1; i < testNodes.size(); i++) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];
		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}

void TestBehaviourTree() {

	float behaviourTimer;
	float distanceToTarget;

	BehaviourAction* findKey = new BehaviourAction("Find Key", [&](float dt, BehaviourState state) -> BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for a key!\n";
			behaviourTimer = rand() % 100;
			state = Ongoing;
		} else if (state == Ongoing) {
			behaviourTimer -= dt;
			if (behaviourTimer <= 0.0f) {
				std::cout << "Found a key!\n";
				return Success;
			}
		}
		return state;
	});

	BehaviourAction* goToRoom = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state) -> BehaviourState {
		if (state == Initialise) {
			std::cout << "Going to loot the room!\n";
			state = Ongoing;
		} else if (state == Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << "Reached Room!\n";
				return Success;
			}
		}
		return state;
	});

	BehaviourAction* openDoor = new BehaviourAction("Open Door", [&](float dt, BehaviourState state) -> BehaviourState {
		if (state == Initialise) {
			std::cout << "Opening Door!\n";
			return Success;
		}
		return state;
	});

	BehaviourAction* lookForTreasure = new BehaviourAction("Look For Treasure", [&](float dt, BehaviourState state) -> BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for treasure!\n";
			return Ongoing;
		} else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some treasure!\n";
				return Success;
			}
			std::cout << "No treasure here...\n";
			return Failure;
		}
		return state;
	});

	BehaviourAction* lookForItems = new BehaviourAction("Look For Items", [&](float dt, BehaviourState state) -> BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for items!\n";
			return Ongoing;
		} else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some items!\n";
				return Success;
			}
			std::cout << "No items here...\n";
			return Failure;
		}
		return state;
	});

	BehaviourSequence* sequence = new BehaviourSequence("Room Selector");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSequence* selection = new BehaviourSequence("Loot Selector");
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Selector");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 5; i++) {
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		
		std::cout << "We're going on an adventure!\n";
		while (state == Ongoing) state = rootSequence->Execute(1.0f);

		if (state == Success) std::cout << "What a successful advanture!\n";
		if (state == Failure) std::cout << "What a waste of time!\n";
	}

	std::cout << "All Done!\n";

}

class PauseScreen : public PushdownState {
	
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::U)) return PushdownResult::Pop;
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		std::cout << "Press U to unpause game!\n";
	}
};

class GameScreen : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		
		pauseReminder -= dt;
		
		if (pauseReminder < 0) {
			std::cout << "Coins minded: " << coinsMined << "\n";
			std::cout << "Press P to pause game, or F1 to return to main menu!\n";
			pauseReminder += 1.0f;
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::P)) {
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}

		if (Window::GetKeyboard()->KeyDown(KeyCodes::F1)) {
			std::cout << "Returning to main menu!\n";
			return PushdownResult::Pop;
		}

		if (rand() % 7 == 0) coinsMined++;
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		std::cout << "Preparing to mine coins!\n";
	}

protected:
	int coinsMined = 0;
	float pauseReminder = 1.0f;
};

class IntroScreen : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
			*newState = new GameScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		std::cout << "Welcome to a really awsome game!\n";
		std::cout << "Press SPACE to begin or ESCAPE to quit!\n";
	}
};

void TestPushdownAutomata(Window* w) {
	PushdownMachine machine(new IntroScreen());
	while (w->UpdateWindow()) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (!machine.Update(dt)) return;
	}
}

class StringPacketReceiver : public PacketReceiver {
public:
	StringPacketReceiver(std::string name) {
		this->name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source) override {
		if (type == String_Message) {
			StringPacket* realPacket = (StringPacket*)payload;
			std::string msg = realPacket->GetStringFromData();
			std::cout << name << " received message: " << msg << std::endl;
		}
	}
protected:
	std::string name;
};

/*
void TestNetworking() {
	NetworkBase::Initialise();

	TestPacketReceiver serverReceiver("Server");
	TestPacketReceiver clientReceiver("Client");

	int port = NetworkBase::GetDefaultPort();

	GameServer* server = new GameServer(port, 1);
	GameClient* client = new GameClient();

	server->RegisterPacketHandler(String_Message, &serverReceiver);
	client->RegisterPacketHandler(String_Message, &clientReceiver);

	bool canConnect = client->Connect(127, 0, 0, 1, port);

	for (int i = 0; i < 100; i++) {
		server->SendGlobalPacket((GamePacket&)StringPacket("Server says hello! " + std::to_string(i)));

		client->SendPacket((GamePacket&)StringPacket("Client says hello! " + std::to_string(i)));

		server->UpdateServer();
		client->UpdateClient();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	NetworkBase::Destroy();
}
*/

class winnerState : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("WINNER", Vector2(45, 45), Debug::RED);
		Debug::Print("Press ENTER for Main Menu", Vector2(30, 50), Debug::RED);

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN)) return PushdownResult::Pop;

		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		this->SetState(3);
		Debug::Print("WINNER", Vector2(45, 45), Debug::RED);
		Debug::Print("Press ENTER for Main Menu", Vector2(30, 50), Debug::RED);
	}
};

class loserState : public PushdownState {
	
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("GAME OVER", Vector2(45, 45), Debug::RED);
		Debug::Print("Press ENTER for Main Menu", Vector2(30, 50), Debug::RED);

		if(Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN)) return PushdownResult::Pop;

		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		this->SetState(2);
		Debug::Print("GAME OVER", Vector2(45, 45), Debug::RED);
		Debug::Print("Press ENTER for Main Menu", Vector2(30, 50), Debug::RED);
	}
};

class gameState : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("Timer: ", Vector2(1, 5), Debug::RED);
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		this->SetState(1);
		Debug::Print("Timer: ", Vector2(1, 5), Debug::RED);
	}
};

class highScoreTable : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		Debug::Print("Check Terminal", Vector2(37, 47), Debug::RED);
		Debug::Print("Hit Enter to return to Main Menu", Vector2(20, 53), Debug::RED);

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN)) {
			return PushdownResult::Pop;
		}

		return PushdownResult::NoChange;

	}

	void OnAwake() override {
		this->SetState(5);
		Debug::Print("Check Terminal", Vector2(37, 47), Debug::RED);
		Debug::Print("Hit Enter to return to Main Menu", Vector2(20, 53), Debug::RED);
	}
};

class mainMenu : public PushdownState {

	PushdownResult OnUpdate(float dt, PushdownState** newState) override {

		Debug::Print("Maze Running", Vector2(37, 40), Debug::BLACK);
		Debug::Print("A: 1 Player", Vector2(37, 47), Debug::RED);
		Debug::Print("B: High Scores", Vector2(37, 53), Debug::RED);
		Debug::Print("To quit press ESC", Vector2(33, 60), Debug::BLACK);

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::A)) {
			*newState = new gameState();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::B)) {
			*newState = new highScoreTable();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		this->SetState(0);
		Debug::Print("Maze Running", Vector2(37, 40), Debug::BLACK);
		Debug::Print("A: 1 Player", Vector2(37, 47), Debug::RED);
		Debug::Print("B: 2 Player", Vector2(37, 53), Debug::RED);
		Debug::Print("To quit press ESC", Vector2(33, 60), Debug::BLACK);
	}
};

PushdownMachine Menu(new mainMenu());

vector<float> scores {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f };

void readHighScore() {
	
	int i = 0;
	std::string temp;
	std::ifstream highScore;
	highScore.open("highScore.txt");

	while(getline(highScore, temp)) {
		if(i < 5) return;
		scores[i] = std::stof(temp);
		i++;
	}

	highScore.close();
}

void addHighScore(float score) {
	
	readHighScore();

	for (int i = 0; i < 5; i++) {
		if(score < scores[i] || scores[i] == -1.0f) {
			scores[i] = score;
			break;
		}
	}

	std::ofstream myfile;
	myfile.open("highScore.txt");
	for(int i = 0; i < 5; i++) myfile << scores[i] << "\n";
	myfile.close();

}

class ScorePacketReceiver : public PacketReceiver {
public:
	ScorePacketReceiver(std::string name) {
		this->name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source) override {
		std::cout << type << std::endl;
		if (type == BasicNetworkMessages::Score) {
			std::cout << type << std::endl;
			ScorePacket* realPacket = (ScorePacket*)payload;
			float score = realPacket->GetScoreData();
			addHighScore(score);
			std::cout << name << " received message: " << std::to_string(score) << std::endl;
		}
	}
protected:
	std::string name;
};

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 725);

	if (!w->HasInitialised()) {
		return -1;
	}

	//TestPathfinding();
	//TestBehaviourTree();
	//TestPushdownAutomata(w);
	//TestNetworking();

	NetworkBase::Initialise();

	ScorePacketReceiver serverReceiver("Server");
	StringPacketReceiver playerReceiver("Player");

	GameServer* Server;
	GameClient* Player;
	int port = NetworkBase::GetDefaultPort();

	Server = new GameServer(port, 1);
	Player = new GameClient();

	bool canConnect = Player->Connect(127, 0, 0, 1, port);

	Server->RegisterPacketHandler(Score, &serverReceiver);
	Player->RegisterPacketHandler(String_Message, &playerReceiver);

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	TutorialGame* g = new TutorialGame();
	w->GetTimer().GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::T)) {
			w->SetWindowPosition(0, 0);
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		Menu.Update(dt);

		switch(Menu.GetActiveState()->GetSate()){
			case 0: g->SetState(GameState::menu); break;
			case 1: g->SetState(GameState::game); break;
			case 2: g->SetState(GameState::lose); break;
			case 3: g->SetState(GameState::win); break;
			case 5: g->SetState(GameState::highScore); break;
			case 6: g->SetState(GameState::save); break;
			default: g->SetState(GameState::menu); break;
		}

		if (g->GetState() == GameState::highScore) {
			readHighScore();
			Server->SendGlobalPacket((GamePacket&)StringPacket("\n1: " + std::to_string(scores[0]) + "\n2: " + std::to_string(scores[1]) +
			"\n3: " + std::to_string(scores[2]) + "\n4: " + std::to_string(scores[3]) + "\n5: " + std::to_string(scores[4])));
			Server->UpdateServer();
			Player->UpdateClient();
		}

		g->UpdateGame(dt);

		if (g->GetState() == GameState::lose) Menu.SetActiveState(new loserState());
		if (g->GetState() == GameState::win || g->GetState() == GameState::save) {
			if (g->GetState() == GameState::save) {
				Player->SendPacket((GamePacket&)ScorePacket(g->GetTimer()));
				Player->UpdateClient();
				Server->UpdateServer();
			}
			Menu.SetActiveState(new winnerState());
		}
		//DisplayPathfinding();
	}
	Window::DestroyGameWindow();
}