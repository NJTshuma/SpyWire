#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;
// declaring and intialising the size of the window

enum Agent8State
{
	STATE_APPEAR,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD,
};
//Constants for the state of Agent 8 are given the values 0 to 3

struct GameState
{
	int score = 0;
	int oldScore = 0;
	//values for the player's score and their score before it last went up or down
	Agent8State agentState = STATE_APPEAR;
	//Agent 8 is initially in the "appear" state
	int nuke = 0;
	int lives = 3;
	float timer = 3;
	int difficulty = 100;
	int level = 1;
	Vector2D shakePos = { 0,50 };
};

GameState gameState;

enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED,
};

void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCoinsAndStars();
void UpdateLasersAndNuke();
void UpdateDestroyed();
void UpdateAgent8();
void MyDrawObject(GameObject& obj);
bool CheckLevel();

// The entry point for a PlayBuffer program
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");
	Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");
	Play::GetGameObject(id_fan).velocity = { 0,3 };
	Play::GetGameObject(id_fan).animSpeed = 1.0f;
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::DrawBackground();
	UpdateAgent8();
	UpdateFan();
	UpdateTools();
	UpdateCoinsAndStars();
	UpdateLasersAndNuke();
	UpdateDestroyed();
	
	Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN AND SPACE TO FIRE",
		{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);
	Play::DrawFontText("132px", "SCORE: " + std::to_string(gameState.score),
		{ DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
	//Play::DrawFontText("132px", std::to_string(gameState.difficulty),{ 50, 50 }, Play::LEFT);
	//Play::DrawFontText("132px", std::to_string(gameState.level),{ 50, 150 }, Play::LEFT);
	Play::DrawFontText("64px", "LIVES: " + std::to_string(gameState.lives),
		{ DISPLAY_WIDTH / 2 - 100, 120 }, Play::CENTRE);
	Play::DrawRect({ DISPLAY_WIDTH / 2 + 10, 100 }, { DISPLAY_WIDTH / 2 + 110, 140 }, Play::cBlack, false); // energy counter container

	if (gameState.nuke == 10)
	{
		Play::DrawFontText("64px", "PRESS E",
			{ DISPLAY_WIDTH / 2 + 27, 120 }, Play::LEFT);
	}

	if (gameState.agentState == STATE_DEAD)
		Play::DrawFontText("132px", "PRESS SPACE TO TRY AGAIN",
			{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);

	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void HandlePlayerControls()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	if (Play::KeyDown(VK_UP))
	{
		obj_agent8.velocity = { 0, -4 };
		Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
	}
	else if (Play::KeyDown(VK_DOWN))
	{
		obj_agent8.acceleration = { 0, 1 };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
	}
	else
	{
		if (obj_agent8.velocity.y > 5)
		{
			gameState.agentState = STATE_HALT;
			Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
			obj_agent8.acceleration = { 0, 0 };
		}
		else
		{
			Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
			obj_agent8.velocity *= 0.5f;
			obj_agent8.acceleration = { 0, 0 };
		}
	}
	if (Play::KeyPressed(VK_SPACE))
	{
		Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);
		int id = Play::CreateGameObject(TYPE_LASER, firePos, 30, "laser");
		Play::GetGameObject(id).velocity = { 32, 0 };
		Play::PlayAudio("shoot");
	}
}

void UpdateFan()
{
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	if (Play::RandomRoll(gameState.difficulty) == gameState.difficulty)
	{
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);

		if (Play::RandomRoll(2) == 1)
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
		}
		Play::PlayAudio("tool");
	}
	if (gameState.difficulty > 50 && CheckLevel())
	{
		gameState.oldScore = gameState.score;
		gameState.difficulty -= 10;						//increases the difficulty of the game (higher probability of tools from the fan)
	}
	if (Play::RandomRoll(150) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;
	}
	Play::UpdateGameObject(obj_fan);
	if (Play::IsLeavingDisplayArea(obj_fan))
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}
	Play::DrawObject(obj_fan);
}

void UpdateTools()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	if (gameState.timer <= 0)
	{
		gameState.timer = 3;
	}
	for (int id : vTools)
	{
		GameObject& obj_tool = Play::GetGameObject(id);

		if (Play::IsColliding(obj_tool, obj_agent8) && gameState.timer == 3 && gameState.agentState != STATE_DEAD)	// make Agent 8 invulnerable for 3 seconds after hit
		{
			gameState.lives -= 1;
			gameState.timer -= 0.017;
			Play::PlayAudio("hit");
		}

		if (gameState.agentState != STATE_DEAD && gameState.lives == 0)
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			gameState.agentState = STATE_DEAD;
		}
		Play::UpdateGameObject(obj_tool);

		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}
		Play::DrawObjectRotated(obj_tool);

		if (!Play::IsVisible(obj_tool))
			Play::DestroyGameObject(id);
	}
	if (gameState.timer < 2.99)						// make Agent 8 invulnerable for 3 seconds after hit
	{
		gameState.timer -= 0.017;
	}
}

void UpdateCoinsAndStars()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_coin : vCoins)
	{
		GameObject& obj_coin = Play::GetGameObject(id_coin);
		bool hasCollided = false;

		if (Play::IsColliding(obj_coin, obj_agent8) && gameState.agentState != STATE_DEAD)
		{
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = { 0.0f, 0.5f };
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}

			hasCollided = true;
			gameState.oldScore = gameState.score;
			gameState.score += 500;
			Play::PlayAudio("collect");
		}

		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		if (!Play::IsVisible(obj_coin) || hasCollided)
			Play::DestroyGameObject(id_coin);
	}

	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

	for (int id_star : vStars)
	{
		GameObject& obj_star = Play::GetGameObject(id_star);

		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);

		if (!Play::IsVisible(obj_star))
			Play::DestroyGameObject(id_star);
	}
}

void UpdateLasersAndNuke()
{
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_laser : vLasers)
	{
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		bool hasCollided = false;
		for (int id_tool : vTools)
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			if (Play::IsColliding(obj_laser, obj_tool))
			{
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.oldScore = gameState.score;
				gameState.score += 100;
				if (gameState.nuke < 10)
				{
					gameState.nuke++;
				}
			}
		}

		for (int id_coin : vCoins)
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			if (Play::IsColliding(obj_laser, obj_coin))
			{
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.oldScore = gameState.score;
				gameState.score -= 300;
			}
		}
		if (gameState.score < 0)
			gameState.score = 0;

		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);

		if (!Play::IsVisible(obj_laser) || hasCollided)
			Play::DestroyGameObject(id_laser);
	}

	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	GameObject& obj_coin = Play::GetGameObjectByType(TYPE_COIN);

	if (Play::KeyPressed(0x45) == true && gameState.nuke >= 10 && gameState.agentState != STATE_DEAD)
	{
		for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
			Play::GetGameObject(id_obj).type = TYPE_DESTROYED;

		MyDrawObject(obj_agent8);
		MyDrawObject(obj_fan);

		for (int id_coin : vCoins)
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			MyDrawObject(obj_coin);
		}

		Play::PlayAudio("explode");

		gameState.nuke = 0;
	}

	Play::DrawRect({ DISPLAY_WIDTH / 2 + 10 , 100 }, { DISPLAY_WIDTH / 2 + 10 + (10 * gameState.nuke), 140 }, Play::cBlack, true); // energy counter
}

void UpdateDestroyed()
{
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	for (int id_dead : vDead)
	{
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);

		if (obj_dead.frame % 2)
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);

		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10)
			Play::DestroyGameObject(id_dead);
	}
}

void UpdateAgent8()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	switch (gameState.agentState)
	{
	case STATE_APPEAR:
		obj_agent8.velocity = { 0, 12 };
		obj_agent8.acceleration = { 0, 0.5f };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
		obj_agent8.rotation = 0;
		if (obj_agent8.pos.y >= DISPLAY_HEIGHT / 3)
			gameState.agentState = STATE_PLAY;
		break;
	case STATE_HALT:
		obj_agent8.velocity *= 0.9f;
		if (Play::IsAnimationComplete(obj_agent8))
			gameState.agentState = STATE_PLAY;
		break;
	case STATE_PLAY:
		HandlePlayerControls();
		break;
	case STATE_DEAD:
		obj_agent8.acceleration = { -0.3f , 0.5f };
		obj_agent8.rotation += 0.25f;

		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.agentState = STATE_APPEAR;
			obj_agent8.pos = { 115, 0 };
			obj_agent8.velocity = { 0, 0 };
			obj_agent8.frame = 0;
			Play::StartAudioLoop("music");
			gameState.lives = 3;
			gameState.score = 0;
			gameState.timer = 3;
			gameState.nuke = 0;
			gameState.difficulty = 100;
			gameState.level = 1;

			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
		}
		break;
	}
	Play::UpdateGameObject(obj_agent8);
	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD)
		obj_agent8.pos = obj_agent8.oldPos;
	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);

	if (((gameState.timer < 2.5 && gameState.timer > 2) || (gameState.timer < 1.5 && gameState.timer > 1) || gameState.timer < 0.5)
		&& gameState.agentState != STATE_DEAD)          // Show "LIFE -1" on Agent 8 every 0.5 seconds for 0.5 seconds
	{
		Play::DrawFontText("64px", "LIFE -1",
			{ obj_agent8.pos }, Play::CENTRE);
	}
}

void MyDrawObject(GameObject& obj)								//for objects to shake in the explosion
{
	for (int i = 0; i < 2; i++)
	{
		Play::DrawSprite(obj.spriteId, obj.pos + gameState.shakePos, obj.frame);
		Play::DrawSprite(obj.spriteId, obj.pos - gameState.shakePos, obj.frame);
	}
}

bool CheckLevel()											//check if the score has passed every 1000 mark
{
	if (gameState.oldScore < gameState.level * 1000 && gameState.score >= gameState.level * 1000)
	{
		gameState.level++;
		return true;
	}
	return false;
}