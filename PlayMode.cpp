#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <random>

GLuint game_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > game_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("8_ball_pool.pnct"));
	game_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > game_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("8_ball_pool.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = game_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = game_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*game_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "CueStick"){
			cue_stick = CueStick(&transform);
		} else if (transform.name == "CueBall"){
			game_balls.emplace_back(Ball(&transform, 1.5f, BallType::White));
		} else if (transform.name == "8Ball"){
			game_balls.emplace_back(Ball(&transform, 1.5f, BallType::Black));
		} else if (transform.name.find("Solid") != std::string::npos){
			game_balls.emplace_back(Ball(&transform, 1.5f, BallType::Solid));
		} else if (transform.name.find("Stripe") != std::string::npos){
			game_balls.emplace_back(Ball(&transform, 1.5f, BallType::Stripe));
		} else if (transform.name == "Obstacle"){
			game_balls.emplace_back(Ball(&transform, 2.0f, BallType::Obstacle));
		}
		
	}

	assert(game_balls.size() == 17);

	for (auto& ball : game_balls){
		if (ball.ball_type == BallType::White){
			cue_ball = &ball;
		}

		if (ball.ball_type == BallType::Black){
			eight_ball = &ball;
		}

		if (ball.ball_type == BallType::Obstacle){
			obstacle_ball = &ball;
		}
	}
	assert(cue_ball);
	assert(eight_ball);
	assert(obstacle_ball);

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	camera->transform->position = glm::vec3(0.0f, 0.0f, 200.0f);
	cue_stick_base_rotation = cue_stick.transform->rotation;

	// Add Game Holes
	// Top Left
	game_holes.emplace_back(Hole(glm::vec3(ArenaMin.x + 0.5f, ArenaMax.y, 0.0f)));

	// Bottom Left
	game_holes.emplace_back(Hole(glm::vec3(ArenaMin.x + 0.5f, ArenaMin.y, 0.0f)));

	// Middle Top
	game_holes.emplace_back(Hole(glm::vec3(0.0f, ArenaMax.y + 1.5f, 0.0f)));

	// Middle Bottom
	game_holes.emplace_back(Hole(glm::vec3(0.0f, ArenaMin.y - 1.5f, 0.0f)));

	// Top Right
	game_holes.emplace_back(Hole(glm::vec3(ArenaMax.x - 0.5f, ArenaMax.y, 0.0f)));

	// Bottom Right
	game_holes.emplace_back(Hole(glm::vec3(ArenaMax.x - 0.5f, ArenaMin.y, 0.0f)));

	// Add velocity to non stoping obstacle ball
	obstacle_ball->velocity.x = ObstacleBallSpeed;
	obstacle_ball->velocity.y = ObstacleBallSpeed;

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_Q){ // rotate cue stick counter clockwise
			cc_rotate.downs += 1;
			cc_rotate.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_E) { // rotate cue stick clockwise
			c_rotate.downs += 1;
			c_rotate.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			charge.downs += 1;
			cue_stick.state = CueStickState::Charge;
			return true;
		}

	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_Q){
			cc_rotate.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_E) {
			c_rotate.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			charge.pressed = false;
			cue_stick.state = CueStickState::Shot;
			player_charged_power = cue_stick.charge_power;
			player_shot_angle = glm::radians(cue_stick.angle);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	}
	

	return false;
}

void PlayMode::update(float elapsed) {
	// charge/uncharge powerstick
	if (game_state == GameState::Play)
	{
		if (cue_stick.state == CueStickState::Aim){
			cue_stick.charge_power = 0.0f;
		} else if (cue_stick.state == CueStickState::Charge){
			cue_stick.charge_power = glm::clamp(cue_stick.charge_power + PowerRateChange, 0.0f, CueStickMaxPower);
		} else if (cue_stick.state == CueStickState::Shot){
			cue_stick.charge_power = glm::clamp(cue_stick.charge_power - 2.0f * PowerRateChange, -5.0f, CueStickMaxPower);
			if (cue_stick.charge_power < -4.0f){
				cue_stick.state = CueStickState::Aim;
				shoot_cue_ball(player_charged_power);
			}

		}
	}

	// rotate cue stick around cue ball
	if (game_state == GameState::Play)
	{
		constexpr float PlayerRotationSpeed = 60.0f; // degrees
		constexpr float DistanceCueBallAndStick = 5.0f; // distance between the cue ball and cue stick

		float rotation_direction = 0.0f;
		if (cc_rotate.pressed && !c_rotate.pressed) rotation_direction = 1.0f;
		if (!cc_rotate.pressed && c_rotate.pressed) rotation_direction = -1.0f;

		cue_stick.angle += rotation_direction * PlayerRotationSpeed * elapsed;
		float angle_rad = glm::radians(cue_stick.angle);

		// based on https://stackoverflow.com/questions/43748418/c-move-2d-point-along-angle
		glm::vec2 move = glm::vec2(0.0f);

		move.x = cue_ball->transform->position.x + glm::cos(angle_rad) * (DistanceCueBallAndStick + (cue_stick.charge_power/2.0f));
		move.y = cue_ball->transform->position.y + glm::sin(angle_rad) * (DistanceCueBallAndStick + (cue_stick.charge_power/2.0f));

		cue_stick.transform->position = glm::vec3(move.x, move.y, 0.0f);
		cue_stick.transform->rotation = glm::angleAxis(angle_rad, glm::vec3(0.0f, 0.0f, 1.0f)) * cue_stick_base_rotation;
	}

	//position/velocity update:
	// based on game 5 base code
	{ 
		for (auto &ball : game_balls){
			
			glm::vec3 new_velocity = ball.velocity;
			if (ball.ball_type != BallType::Obstacle){
				float amt = 1.0f - std::pow(0.5f, elapsed / (BallAccelHalflife * 2.0f));
				new_velocity = glm::mix(ball.velocity, glm::vec3(0.0f), amt);
			}

			ball.velocity.x = new_velocity.x;
			ball.velocity.y = new_velocity.y;
			
			ball.transform->position.x += ball.velocity.x * elapsed;
			ball.transform->position.y += ball.velocity.y * elapsed;

			if (ball.is_falling){
				ball.velocity.z += Gravity;
				ball.transform->position.z += ball.velocity.z * elapsed;
			}
		}
	}

	// collision resolution:
	// based on game 5 base code
	{
		for (auto &ball1 : game_balls) {
			// ball/ball collisions:
			for (auto &ball2 : game_balls) {
				if (&ball1 == &ball2) break;
				glm::vec3 p12 = ball2.transform->position - ball1.transform->position;
				float len2 = glm::length2(p12);
				if (len2 > (2.0f * ball1.radius) * (2.0f * ball2.radius)) continue;
				if (len2 == 0.0f) continue;
				glm::vec3 dir = p12 / std::sqrt(len2);
				//mirror velocity to be in separating direction:
				glm::vec3 v12 = ball2.velocity - ball1.velocity;
				glm::vec3 delta_v12 = dir * glm::max(0.0f, -1.75f * glm::dot(dir, v12));
				ball2.velocity += 0.5f * delta_v12;
				ball1.velocity -= 0.5f * delta_v12;
			}

			// ball/arena collisions:
			if (!ball1.is_falling){
				if (ball1.transform->position.x < ArenaMin.x + ball1.radius) {
					ball1.transform->position.x = ArenaMin.x + ball1.radius;
					ball1.velocity.x = std::abs(ball1.velocity.x);
				}

				if (ball1.transform->position.x > ArenaMax.x - ball1.radius) {
					ball1.transform->position.x = ArenaMax.x - ball1.radius;
					ball1.velocity.x =-std::abs(ball1.velocity.x);
				}

				if (ball1.transform->position.y < ArenaMin.y + ball1.radius) {
					ball1.transform->position.y = ArenaMin.y + ball1.radius;
					ball1.velocity.y = std::abs(ball1.velocity.y);
				}

				if (ball1.transform->position.y > ArenaMax.y - ball1.radius) {
					ball1.transform->position.y = ArenaMax.y - ball1.radius;
					ball1.velocity.y =-std::abs(ball1.velocity.y);
				}
			}

			// ball/table collisions:
			if (ball1.is_falling){
				if (ball1.transform->position.x < TableMin.x + ball1.radius) {
					ball1.transform->position.x = TableMin.x + ball1.radius;
					ball1.velocity.x = std::abs(ball1.velocity.x);
				}

				if (ball1.transform->position.x > TableMax.x - ball1.radius) {
					ball1.transform->position.x = TableMax.x - ball1.radius;
					ball1.velocity.x =-std::abs(ball1.velocity.x);
				}

				if (ball1.transform->position.y < TableMin.y + ball1.radius) {
					ball1.transform->position.y = TableMin.y + ball1.radius;
					ball1.velocity.y = std::abs(ball1.velocity.y);
				}

				if (ball1.transform->position.y > TableMax.y - ball1.radius) {
					ball1.transform->position.y = TableMax.y - ball1.radius;
					ball1.velocity.y =-std::abs(ball1.velocity.y);
				}
			}

			// ball/hole collision
			// based on https://stackoverflow.com/questions/79787358/how-to-find-if-a-point-is-in-a-circle-in-c
			for (auto& hole : game_holes){
				float dx = ball1.transform->position.x - hole.position.x;
				float dy = ball1.transform->position.y - hole.position.y;
				float distance_squared = dx * dx + dy * dy;
				float radius_squared = hole.radius * hole.radius;

				if (distance_squared <= radius_squared && !ball1.is_falling){
					ball1.is_falling = true;

					if (ball1.ball_type == BallType::White || ball1.ball_type == BallType::Black){
						end_game();
					} else {
						update_score(ball1.ball_type);
					}
				}

			}

		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	cc_rotate.downs = 0;
	c_rotate.downs = 0;
	charge.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);
	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));
		constexpr float H = 0.09f;
		lines.draw_text("Q & E to Rotate Cue Stick. Hold Space to Shoot",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Q & E to Rotate Cue Stick. Hold Space to Shoot",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text(game_message,
			glm::vec3(-0.4f, 0.4f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		
		lines.draw_text("Game Score: " + std::to_string(game_score),
			glm::vec3(-0.4f, 0.7f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text("Shots Left " + std::to_string(shots_left),
			glm::vec3(-1.5f, 0.7f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

void PlayMode::shoot_cue_ball(float charged_power)
{
	// based on https://gamedev.stackexchange.com/questions/117583/how-do-i-get-a-vector-from-an-angle
	glm::vec3 shot_direction = glm::normalize(glm::vec3(glm::cos(player_shot_angle), glm::sin(player_shot_angle), 0.0f));

	cue_ball->velocity = -1.0f * shot_direction * player_charged_power;

	shots_left--;

	if (shots_left <= 0){
		end_game();
	}
}

void PlayMode::update_score(BallType ball_type)
{
	if (ball_type == BallType::Solid){
		game_score += 100;
	} else if (ball_type == BallType::Stripe){
		game_score += 500;
	} else if (ball_type == BallType::Obstacle){
		game_score += 1000;
	}
}

void PlayMode::end_game()
{
	game_state = GameState::PostRound;
	game_message = "Game Over";
}
