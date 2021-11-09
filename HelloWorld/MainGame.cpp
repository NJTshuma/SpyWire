#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;
//The values for the width, height and scale of the game window in pixels are declared and initialised as integers with a global scope

enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD,
};
//These four constants are given the values 0 to 3, giving the values a useful name

struct GameState
{
	int score = 0;							//the user's score is set to 0
	Agent8State agentState = STATE_APPEAR;	//agent 8 is initally set to appear
};
//Data is grouped together in this data structure

GameState gameState;						//a Gamestate is initialised

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
//Values are assigned to each of these types of "objects" in the game

void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCoinsAndStars();
void UpdateLasers();
void UpdateDestroyed();
void UpdateAgent8();
//The functions are declared here so that they can be used in MainGameUpdate 

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );		//Creates the game window
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");
	Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");				//Loads in Agent 8
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");		//Loads in the fan
	Play::GetGameObject(id_fan).velocity = { 0, 3 };							//The fan moves vertically					
	Play::GetGameObject(id_fan).animSpeed = 1.0f;								//It is animated at 60fps
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	Play::DrawBackground();
	UpdateAgent8();
	HandlePlayerControls();
	UpdateFan();
	UpdateTools();
	UpdateCoinsAndStars();
	UpdateLasers();
	UpdateDestroyed();				//each function is called with the order determining which ones are drawn on top
	Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN AND SPACE TO FIRE", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);
	Play::DrawFontText("132px", "SCORE: " + std::to_string(gameState.score), { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
	Play::PresentDrawingBuffer();		//displays everything that was drawn above this line
	return Play::KeyDown( VK_ESCAPE );  //closes the game when you press the escape key
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();			//frees up the memory as the application closes
	return PLAY_OK;					//returns PLAY_OK if it closes successfuly
}

void HandlePlayerControls()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);		//creates an object for agent 8
	if (gameState.agentState != STATE_DEAD)			//if the player hasn't lost
	{
		if (Play::KeyDown(VK_UP))
		{
			obj_agent8.velocity = { 0, -4 };
			Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
		}		//when the up key is pressed agent 8 goes up with a climbing animation
		else if (Play::KeyDown(VK_DOWN))
		{
			obj_agent8.acceleration = { 0, 1 };
			Play::SetSprite(obj_agent8, "agent8_fall", 0);
		}		//when the down key is pressed agent 8 accelerates downwards as the falling sprite
		else
		{
			if (obj_agent8.velocity.y > 5)
			{
				gameState.agentState = STATE_HALT;
				Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
				obj_agent8.acceleration = { 0, 0 };
			}		//if the player stops moving and the velocity was greater than 5, agent 8 stops and plays a halt animation
			else
			{
				Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
				
				obj_agent8.velocity *= 0.5f;
				obj_agent8.acceleration = { 0, 0 };
			}		//the player stops moving and velocity was less than 5, agent 8 eases to a stop and plays the hang animation
		}
		if (Play::KeyPressed(VK_SPACE))
		{
			Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);		//the position of the laser is defined here in relation to the centre of agent 8
			int id = Play::CreateGameObject(TYPE_LASER, firePos, 30, "laser");		//the laser is created and given an id
			Play::GetGameObject(id).velocity = { 32, 0 };		//the laser travels to the right
			Play::PlayAudio("shoot");
		}
		Play::UpdateGameObject(obj_agent8);		//the objects are updated based on the player's input

		if (Play::IsLeavingDisplayArea(obj_agent8))
			obj_agent8.pos = obj_agent8.oldPos;			//if agent 8 is going outside the window, it's position will be reset to the last position before it "left"
	}

	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);		//the web is drawn increasing in length based on agent 8's position
	Play::DrawObjectRotated(obj_agent8);		//draws agent 8 and allows it to spin when it get's hit
}

void UpdateFan()
{
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);		//creates an object for the fan
	if (Play::RandomRoll(50) == 50)		//a 1/50 chance in each loop
	{
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);		//a screwdriver tool is created at the centre of the fan
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);		//the y velocity has a random range from -6 to 6

		if (Play::RandomRoll(2) == 1)		//a half chance of the 1/50 chance
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;		//not sure what the radius is
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
		}		//the spanners spin towards the player
		Play::PlayAudio("tool");
	}
	if (Play::RandomRoll(150) == 1)		//a 1/150 chance
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;
	}		//the coins spin towards the player
	Play::UpdateGameObject(obj_fan);

	if (Play::IsLeavingDisplayArea(obj_fan))
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;		//the fan changes direction when it gets to the bottom or top of the screen
	}
	Play::DrawObject(obj_fan);
}

void UpdateTools()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);	//agent 8 is retrieved here to interact with the tools in this function
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);		//creates a vector which stores the ids of all of the tools

	for (int id : vTools)	//this repeats for all the tools that are on screen
	{
		GameObject& obj_tool = Play::GetGameObject(id);		//a tool object is created

		if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_tool, obj_agent8)) {
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			gameState.agentState = STATE_DEAD;
		}		//if agent 8 collides with a tool, the death sound plays, the music stops and it goes into the dead state
		Play::UpdateGameObject(obj_tool);		//the tool is updated

		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}		//the tools bounce on the top and bottom of the screen
		Play::DrawObjectRotated(obj_tool);

		if (!Play::IsVisible(obj_tool))
			Play::DestroyGameObject(id);
	}		//if the tool can't be seen it is destroyed. Outside of the window?
}

void UpdateCoinsAndStars()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);		//agent 8 is retrieved here to interact with the coins in this function
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);		//creates a vector which stores the ids of all of the coins

	for (int id_coin : vCoins)		//this repeats for all the tools that are on screen
	{
		GameObject& obj_coin = Play::GetGameObject(id_coin);		//a tool object is created
		bool hasCollided = false;		//a boolean variable to detect a collision and make sure it only registers once

		if (Play::IsColliding(obj_coin, obj_agent8)) //if a coin touches agent 8
		{
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)		//gives four different angles
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");		//a star object is created that starts at agent 8's position
				GameObject& obj_star = Play::GetGameObject(id);		//the star object is retrieved
				obj_star.rotSpeed = 0.1f;		//the star spins at 0.1 rotations per loop
				obj_star.acceleration = { 0.0f, 0.5f };		//"gravity"
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);		//shoots the star out in a direction given by the for loop
			}

			hasCollided = true;		//collision detected
			gameState.score += 500;		//increase user score by 500
			Play::PlayAudio("collect");
		}

		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		if (!Play::IsVisible(obj_coin) || hasCollided)
			Play::DestroyGameObject(id_coin);
	}		//the coin is destroyed if it collides with agent 8 or it is off screen

	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);		//creates a vector which stores the ids of all of the coins

	for (int id_star : vStars)
	{
		GameObject& obj_star = Play::GetGameObject(id_star);

		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);

		if (!Play::IsVisible(obj_star))
			Play::DestroyGameObject(id_star);
	}		//this loop updates the stars and destroys them when they are off screen
}

void UpdateLasers()
{
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);		//creates a vector which stores the ids of all of the lasers
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);		//creates a vector which stores the ids of all of the tools
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);		//creates a vector which stores the ids of all of the coins

	for (int id_laser : vLasers)		//repeats for all of the lasers
	{
		GameObject& obj_laser = Play::GetGameObject(id_laser);		//a laser object is retrieved
		bool hasCollided = false;		//a boolean variable to detect a collision and make sure it only registers once
		for (int id_tool : vTools)		//repeats for all of the tools
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			if (Play::IsColliding(obj_laser, obj_tool))
			{
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score += 100;
			}		//when the laser hits a tool, the score goes up by 100
		}

		for (int id_coin : vCoins)		//repeats for all of the coins
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			if (Play::IsColliding(obj_laser, obj_coin))
			{
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
			}		//if the laser hits a coin, the score goes down by 100 and an error audio is played
		}
		if (gameState.score < 0)
			gameState.score = 0;		//resets the score to 0 if it becomes negative

		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);

		if (!Play::IsVisible(obj_laser) || hasCollided)
			Play::DestroyGameObject(id_laser);
	}		//the laser is destroyed if it collides with agent 8 or it is off screen
}

void UpdateDestroyed()
{
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	for (int id_dead : vDead)		//repeats for all of the destroyed objects
	{
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);

		if (obj_dead.frame % 2)
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);

		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10)
			Play::DestroyGameObject(id_dead);
	}		//destroys dead objects if they have faded out for at least 10 frames or they are out of the window
}

void UpdateAgent8()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);		//retrieves agent 8

	switch (gameState.agentState)		//checks to see which state agent 8 is in and runs the case that matches.
	{
	case STATE_APPEAR:
		obj_agent8.velocity = { 0, 12 };
		obj_agent8.acceleration = { 0, 0.5f };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
		obj_agent8.rotation = 0;
		if (obj_agent8.pos.y >= DISPLAY_HEIGHT / 3)
			gameState.agentState = STATE_PLAY;
		break;		//agent 8 is dropped in to the window until it is a third of the way down before giving the player control

	case STATE_HALT:
		obj_agent8.velocity *= 0.9f;
		if (Play::IsAnimationComplete(obj_agent8))
			gameState.agentState = STATE_PLAY;
		break;		//slows down agent 8 and makes sure the halt animation is complete before giving the player control

	case STATE_PLAY:
		HandlePlayerControls();
		break;		//allows the player to control agent 8

	case STATE_DEAD:		//for when the player loses
		obj_agent8.acceleration = { -0.3f , 0.5f };		//accelerates agent 8 down and to the left
		obj_agent8.rotation += 0.25f;		//causes agent 8 to spin
		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.agentState = STATE_APPEAR;
			obj_agent8.pos = { 115, 0 };
			obj_agent8.velocity = { 0, 0 };
			obj_agent8.frame = 0;
			Play::StartAudioLoop("music");
			gameState.score = 0;

			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
		}		//when spacebar is pressed after losing, the game is reset and the tools on screen are destroyed.
		break;

	} // End of switch on Agent8State

	Play::UpdateGameObject(obj_agent8);

	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD)
		obj_agent8.pos = obj_agent8.oldPos;		//stops agent 8 from leaving the screen by setting it to the last position on the screen

	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}