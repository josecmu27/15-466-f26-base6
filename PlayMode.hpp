#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

enum GameState {
	Play,
	PostRound
};

enum BallType {
	Solid,
	Stripe,
	Black,
	White,
	Obstacle
};

enum CueStickState {
	Aim,
	Charge,
	Shot
};

struct Ball {
	Scene::Transform* transform = nullptr;
	glm::vec3 velocity = glm::vec3(0.0f);
	float radius = 1.5f;
	BallType ball_type;
	bool is_falling = false;

	Ball(Scene::Transform* transform, float radius, BallType ball_type) : 
		transform(transform), radius(radius), ball_type(ball_type) {};
};

struct CueStick {
	Scene::Transform* transform = nullptr;
	glm::vec3 velocity = glm::vec3(0.0f);
	float charge_power = 0.0f;
	float radius = 10.0f;
	float angle = 0.0f;
	CueStickState state = CueStickState::Aim;

	CueStick(Scene::Transform* transform) : 
		transform(transform) {};
};

struct Hole {
	glm::vec3 position = glm::vec3(0.0f);
	float radius = 4.0f;

	Hole(glm::vec3 position) : 
		position(position) {};
};


struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, cc_rotate, c_rotate, charge;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	CueStick cue_stick = CueStick(nullptr);

	std::vector<Ball> game_balls;
	std::vector<Hole> game_holes; 

	Ball* cue_ball = nullptr;
	Ball* eight_ball = nullptr;
	Ball* obstacle_ball = nullptr;

	glm::quat cue_stick_base_rotation;
	float player_charged_power = 0.0f;
	float player_shot_angle = 0.0f; // radians

	GameState game_state = GameState::Play;
	int game_score = 0;
	int shots_left = 10;
	std::string game_message = "";

	/*------------------Constants-------------------*/
	float PowerRateChange = 1.0f;
	float CueStickMaxPower = 50.0f;
	float BallAccelHalflife = 1.0f;
	float ObstacleBallSpeed = 10.0f;
	float Gravity = -2.5f;

	glm::vec2 ArenaMin = glm::vec2(-51.904f, -25.2637f);
	glm::vec2 ArenaMax = glm::vec2(51.904f,  25.2637f);

	glm::vec2 TableMin = glm::vec2(-55.0f, -31.5f);
	glm::vec2 TableMax = glm::vec2(55.0f, 31.5f);
	
	//camera:
	Scene::Camera *camera = nullptr;

	void shoot_cue_ball(float charged_power);

	void update_score(BallType ball_type);

	void end_game();
};
