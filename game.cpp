#include<algorithm>

#include "game.h"
#include "spriterenderer.h"
#include "resourceManager.h"
#include "gameobject.h"
#include "playerObject.h"
#include "particlegenerator.h"
#include "postprocessor.h"
#include <irr/irrKlang.h>
#include <iostream>
using namespace irrklang;

//game related state data
SpriteRenderer* Renderer;
PlayerObject* shiro;  //the dog in spaceship
PlayerObject* swimShiro;
//PlayerObject* shark, *shark2;  //sharks in the ocean
ParticleGenerator* Particles;
PostProcessor* Effects;
ISoundEngine* SoundEngine = createIrrKlangDevice();
float ShakeTime = 0.0f;
float sharkRenderTimer = 0.0f;
const float sharkRenderDelay = 3.0f; // Adjust this value to set the delay in seconds
bool rendersharks = false;
//data related to the homepage, positions of the textures
float x_postion = 520.0f;
float y_postion = 200.0f;
float x_width = 120.0f;
float y_width = 80.0f;
float gap = 120.0f;
std::vector<shark> sharks; //vector to hold shark objects

Game::Game(unsigned int width, unsigned int height) 
	:State(GAME_MENU), Keys(), Width(width), Height(height),Lives(3) {

}

Game::~Game() {
	delete Renderer;
    delete shiro;
    delete Particles;
    delete Effects;
    delete swimShiro;
    sharks.clear();

    SoundEngine->drop();
}

void Game::Init() {
    // load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", nullptr, "particle");
    ResourceManager::LoadShader("shaders/postprocessing.vs", "shaders/postprocessing.frag", nullptr, "postprocessing");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
        static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);

    // load textures
    ResourceManager::LoadTexture("textures/spacebg.jpg", false, "background");
    ResourceManager::LoadTexture("textures/ocean.png", true, "ocean");
    ResourceManager::LoadTexture("textures/controls.png", true, "helpmenu");
    ResourceManager::LoadTexture("textures/spaceship.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/astroids.png", true, "block_solid");
    ResourceManager::LoadTexture("textures/particle.png", true, "particle");
    ResourceManager::LoadTexture("textures/passed.png", true, "passed");
    ResourceManager::LoadTexture("textures/wasted.png", true, "wasted");
    ResourceManager::LoadTexture("textures/swim.png", true, "swim");
    ResourceManager::LoadTexture("textures/lvl1purple.png", true, "lvl1p");
    ResourceManager::LoadTexture("textures/lvl2purple.png", true, "lvl2p");
    ResourceManager::LoadTexture("textures/lvl1blue.png", true, "lvl1b");
    ResourceManager::LoadTexture("textures/lvl2blue.png", true, "lvl2b");
    ResourceManager::LoadTexture("textures/helppurple.png", true, "helpp");
    ResourceManager::LoadTexture("textures/helpblue.png", true, "helpb");
    ResourceManager::LoadTexture("textures/exitpurple.png", true, "exitp");
    ResourceManager::LoadTexture("textures/exitblue.png", true, "exitb");
    ResourceManager::LoadTexture("textures/shark.png", true, "sharkright");
    ResourceManager::LoadTexture("textures/sharkleft.png", true, "sharkleft");
    //setting shark controls
    Shader spriteShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(spriteShader);
    //// Load shark texture
    Texture2D sharkTextureright = ResourceManager::GetTexture("sharkright");
    Texture2D sharkTextureleft = ResourceManager::GetTexture("sharkleft");


        // set render-specific controls
    Shader theShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(theShader);

    Particles = new ParticleGenerator(
        ResourceManager::GetShader("particle"),
        ResourceManager::GetTexture("particle"),
        500);

    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
    // load levels
    GameLevel one; one.Load("levels/one.lvl", this->Width, this->Height*0.8);
    GameLevel two; two.Load("levels/two.lvl", this->Width, this->Height*0.8);
    GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height*0.8);
    GameLevel four; //might be required for exit button
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;

    glm::vec2 shiroPos = glm::vec2(this->Width / 2.0f - BALL_RADIUS, this->Height-BALL_RADIUS*2.0f);
    glm::vec2 sharkPos1(1230.0f, 400.0f);
    glm::vec2 sharkPos2(-20.0f, 300.0f);
    glm::vec2 sharkPos3(1210.0f, 230.0f);
    glm::vec2 sharkPos4(00.0f, 500.0f);
    glm::vec2 sharkPos5(1200.0f, 600.0f);

    shiro = new PlayerObject(shiroPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));
    swimShiro = new PlayerObject(shiroPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("swim"));
    sharks.emplace_back(sharkPos1, sharkRadius, sharkVelocityleft, sharkTextureright);
    sharks.emplace_back(sharkPos2, sharkRadius, sharkVelocityright, sharkTextureleft);
    sharks.emplace_back(sharkPos3, sharkRadius, sharkVelocityleft, sharkTextureright);
    sharks.emplace_back(sharkPos4, sharkRadius, sharkVelocityright, sharkTextureleft);
    sharks.emplace_back(sharkPos5, sharkRadius, sharkVelocityleft, sharkTextureright);


    //audio
    SoundEngine->play2D("audio/breakout.mp3", true);
}

void Game::ProcessInput(float dt) {
    if (this->State == GAME_MENU) {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            if( this->Level != 2)
                this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
        }
        if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
        {
            if (this->levelSelect > 0)
                --this->levelSelect;
            else
                this->levelSelect = 3;

            this->Level = this->levelSelect;

            this->KeysProcessed[GLFW_KEY_W] = true;
        }
        if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
        {
            this->levelSelect = (this->levelSelect + 1) % 4;
            this->Level = this->levelSelect;
            this->KeysProcessed[GLFW_KEY_S] = true;
        }

    }
    if (this->State == GAME_WIN || this->State == GAME_OVER) {
        if (this->Keys[GLFW_KEY_ENTER]) {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            Effects->Chaos = false;
            this->State = GAME_MENU;
        }
    }
    if (this->Level == 2) {
        this->KeysProcessed[GLFW_KEY_ENTER] = false;
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER]) {
            if (this->State == GAME_MENU) {
                std::cout << "Entering Help Menu!" << std::endl;
                this->State = HELP_MENU;
            }
            else if (this->State == HELP_MENU) {
                std::cout << "Exiting Help Menu!" << std::endl;
                this->State = GAME_MENU;
            }
            this->KeysProcessed[GLFW_KEY_ENTER] = true; // Mark the Enter key as processed
        }
    }
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        if (this->Level == 1) {
            for (auto& shark : sharks)
            {
                shark.Position += shark.Velocity * dt;
                if (shark.Position.x + shark.Radius < 0.0f) {
                    shark.Position.x = this->Width + shark.Radius; // Wrap to the right edge
                }
                if (shark.Position.x - shark.Radius > this->Width) {
                    shark.Position.x = -shark.Radius; // Wrap to the left edge
                }
            }
            // Increment the sharkRenderTimer
            sharkRenderTimer += 1 * dt;
            if (sharkRenderTimer >= 4.00f) {
                rendersharks = true;
                sharkRenderTimer = 0.00f;
            }
        }
        
            // move playerboard
            if (this->Keys[GLFW_KEY_LEFT])
            {
                shiro->Position.x -= 1;
                swimShiro->Position.x -= 1;
            }
            if (this->Keys[GLFW_KEY_RIGHT])
            {
                shiro->Position.x += 1;
                swimShiro->Position.x += 1;
            }

            if (this->Keys[GLFW_KEY_SPACE])
                shiro->Stuck = false;

            if (this->Keys[GLFW_KEY_UP]) {
                shiro->Position.y -= 1;
                swimShiro->Position.y -= 1;
            }
            if (this->Keys[GLFW_KEY_DOWN]) {
                shiro->Position.y += 1;
                swimShiro->Position.y += 1;
            }
        }
    }
void Game::Update(float dt) {
    //update objects
    
    if (this->Level == 1) {
        this->DoCollisions();
    }
    else {
        //checks for collisions
        this->DoCollisions();
        //update particles
        Particles->Update(dt, *shiro, 2, glm::vec2(shiro->Radius / 2.0f));
    }
    
    //update powerups
   // this->UpdatePowerUps(dt);
  
    //reduce shake time
    if (ShakeTime > 0.0f) {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = false;
    }

    //check loss condition
    if (shiro->Position.y >= this->Height) //did the ball reach bottom edge
    {
        --this->Lives;
        if (this->Lives == 0) {
            this->ResetLevel();
            this->ResetPlayer();
        }
        this->ResetPlayer();
    }
    //win check
    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        // Check if this is the last level
    
        if (this->Levels[1].IsCompleted())
        {
            // All levels are completed, show win page
            this->ResetLevel();
            this->ResetPlayer();
            Effects->Chaos = true;
            this->State = GAME_WIN;
        }
        else if (this->Level == 3) {
            glfwTerminate();    // Terminate GLFW
            exit(0);          // Exit the program
        }
        else //increase the level
        {
            this->Level++;
            this->ResetPlayer();
        }
    }
}

void Game::Render() {
    Texture2D theTexture;
    if (this->State == GAME_ACTIVE|| this->State == GAME_MENU || this->State == GAME_WIN || this->State == HELP_MENU)
    {
        Effects->BeginRender();

        if (this->State == GAME_MENU) {
            theTexture = ResourceManager::GetTexture("background");
            Renderer->DrawSprite(theTexture, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);

            if (this->levelSelect == 0) {
                theTexture = ResourceManager::GetTexture("lvl1b");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion), glm::vec2(x_width, y_width), 0.0f);
            }
            else {
                theTexture = ResourceManager::GetTexture("lvl1p");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion), glm::vec2(x_width, y_width), 0.0f);
            }

            if (this->levelSelect == 1) {
                theTexture = ResourceManager::GetTexture("lvl2b");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion + gap), glm::vec2(x_width, y_width), 0.0f);
            }
            else { theTexture = ResourceManager::GetTexture("lvl2p");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion + gap), glm::vec2(x_width, y_width), 0.0f);
            }

            if (this->levelSelect == 2) {
                theTexture = ResourceManager::GetTexture("helpb");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion + gap*2), glm::vec2(x_width, y_width), 0.0f);
            }
            else {
                theTexture = ResourceManager::GetTexture("helpp");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion + gap*2), glm::vec2(x_width, y_width), 0.0f);
            }
            if (this->levelSelect == 3) {
                theTexture = ResourceManager::GetTexture("exitb");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion + gap * 3), glm::vec2(x_width, y_width), 0.0f);
            }
            else {
                theTexture = ResourceManager::GetTexture("exitp");
                Renderer->DrawSprite(theTexture, glm::vec2(x_postion, y_postion + gap * 3), glm::vec2(x_width, y_width), 0.0f);
            }
         }
        else if (this->State == HELP_MENU)
        {
            //std::cout << "Help meanu aayoo";
            theTexture = ResourceManager::GetTexture("helpmenu");
            Renderer->DrawSprite(theTexture, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        }

        else {
            // draw background
            if (this->Level == 1) {
                theTexture = ResourceManager::GetTexture("ocean");
                Renderer->DrawSprite(theTexture, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
                for (auto& shark : sharks) {
                    shark.Draw(*Renderer);
                }
                swimShiro->Draw(*Renderer); 
                if (rendersharks) {
                    // Reset the timer
                    std::cout << sharkRenderTimer<<"\n";
                    // Render the sharks
                    for (auto& shark : sharks) {
                        shark.Draw(*Renderer);
                    }
                }
            }
            else {
                theTexture = ResourceManager::GetTexture("background");
                Renderer->DrawSprite(theTexture, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
                //draw particles
                Particles->Draw();
                //draw ball
                shiro->Draw(*Renderer);
            }
            // draw level
            this->Levels[this->Level].Draw(*Renderer);
        }
        // draw powerups
        //for (PowerUp& powerUp : this->PowerUps)
        //    if (!powerUp.Destroyed)
        //        powerUp.Draw(*Renderer);

        //end rendering to postprocessig framebuffer
        Effects->EndRender();
        //render postprocessing quad
        Effects->Render(glfwGetTime());

        if (this->State == GAME_WIN) {
            theTexture = ResourceManager::GetTexture("passed");
            Renderer->DrawSprite(theTexture, glm::vec2(this->Width/4.0f, this->Height/4.0f), glm::vec2(this->Width / 2.0f, this->Height / 2.0f), 0.0f);
        }
    }
    if (this->State == GAME_OVER) {
        theTexture = ResourceManager::GetTexture("wasted");
        Renderer->DrawSprite(theTexture, glm::vec2(this->Width / 4.0f, this->Height / 4.0f), glm::vec2(this->Width / 2.0f, this->Height / 2.0f), 0.0f);
    }
}

void Game::ResetLevel() {
    if (this->Level == 0)
        this->Levels[0].Load("levels/one.lvl", this->Width, this->Height * 0.8);
    else if (this->Level == 1)
        this->Levels[1].Load("levels/two.lvl", this->Width, this->Height * 0.8);
    else if (this->Level == 2)
        this->Levels[2].Load("levels/three.lvl", this->Width, this->Height * 0.8);
    this->Lives = 3;
}

void Game::ResetPlayer() {
    //reset player/ball states

    shiro->Reset(glm::vec2(this->Width / 2.0f - BALL_RADIUS, this->Height - BALL_RADIUS * 2), INITIAL_BALL_VELOCITY);
    swimShiro->Reset(glm::vec2(this->Width / 2.0f - BALL_RADIUS, this->Height - BALL_RADIUS * 2), INITIAL_BALL_VELOCITY);
    // also disable all active powerups
    Effects->Chaos = Effects->Confuse = false;
    shiro->PassThrough = shiro->Sticky = false;
    //Player->Color = glm::vec3(1.0f);
    shiro->Color = glm::vec3(1.0f);
    swimShiro->Color = glm::vec3(1.0f);
}

// powerups
bool IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type);

void Game::UpdatePowerUps(float dt)
{
    for (PowerUp& powerUp : this->PowerUps)
    {
        powerUp.Position += powerUp.Velocity * dt;
        if (powerUp.Activated)
        {
            powerUp.Duration -= dt;

            if (powerUp.Duration <= 0.0f)
            {
                // remove powerup from list (will later be removed)
                powerUp.Activated = false;
                // deactivate effects
                if (powerUp.Type == "sticky")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
                    {	// only reset if no other PowerUp of type sticky is active
                        shiro->Sticky = false;
                        //Player->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "pass-through")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {	// only reset if no other PowerUp of type pass-through is active
                        shiro->PassThrough = false;
                        shiro->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "confuse")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {	// only reset if no other PowerUp of type confuse is active
                        Effects->Confuse = false;
                    }
                }
                else if (powerUp.Type == "chaos")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {	// only reset if no other PowerUp of type chaos is active
                        Effects->Chaos = false;
                    }
                }
            }
        }
    }
    // Remove all PowerUps from vector that are destroyed AND !activated (thus either off the map or finished)
    // Note we use a lambda expression to remove each PowerUp which is destroyed and not activated
    this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
        [](const PowerUp& powerUp) { return powerUp.Destroyed && !powerUp.Activated; }
    ), this->PowerUps.end());
}

bool ShouldSpawn(unsigned int chance)
{
    unsigned int random = rand() % chance;
    return random == 0;
}
void Game::SpawnPowerUps(GameObject& block)
{
    if (ShouldSpawn(75)) // 1 in 75 chance
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
    if (ShouldSpawn(15)) // Negative powerups should spawn more often
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
    if (ShouldSpawn(15))
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}

void ActivatePowerUp(PowerUp& powerUp)
{
    if (powerUp.Type == "speed")
    {
        shiro->Velocity *= 1.2;
    }
    else if (powerUp.Type == "sticky")
    {
        shiro->Sticky = true;
        //Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
        shiro->PassThrough = true;
        shiro->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    }
    else if (powerUp.Type == "pad-size-increase")
    {
       // Player->Size.x += 50;
    }
    else if (powerUp.Type == "confuse")
    {
        if (!Effects->Chaos)
            Effects->Confuse = true; // only activate if chaos wasn't already active
    }
    else if (powerUp.Type == "chaos")
    {
        if (!Effects->Confuse)
            Effects->Chaos = true;
    }
}

bool IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type)
{
    // Check if another PowerUp of the same type is still active
    // in which case we don't disable its effect (yet)
    for (const PowerUp& powerUp : powerUps)
    {
        if (powerUp.Activated)
            if (powerUp.Type == type)
                return true;
    }
    return false;
}


//collision detection
bool CheckCollision(GameObject& one, GameObject& two);
Collision CheckCollision(PlayerObject& one, GameObject& two);
bool CheckSharkCollision(PlayerObject& player, shark& theShark);
Direction VectorDirection(glm::vec2 closest);

void Game::DoCollisions() {
    for (GameObject& box : this->Levels[this->Level].Bricks) {
        if (!box.Destroyed) {
            Collision collision = CheckCollision(*shiro, box);
            if (std::get<0>(collision)) {//if collision is true
                //destroyed block if not solid
                if (!box.IsSolid) {
                    box.Destroyed = true;
                    //this->SpawnPowerUps(box);
                    SoundEngine->play2D("audio/bleep.mp3", false);
                }
                else {
                    //if the ball hits the solid block then we enable the shake effect
                   ShakeTime = 0.02f;
                    Effects->Shake = true;
                    SoundEngine->play2D("audio/solid.wav", false);
                }

                //collision resolution
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (!(shiro->PassThrough && !box.IsSolid)) {
                    if (dir == LEFT || dir == RIGHT) {//horizontal collision
                        shiro->Velocity.x = -shiro->Velocity.x; //reverse horizontal velocity
                        swimShiro->Velocity.x = -swimShiro->Velocity.x;

                        //relocate
                        float penetration = shiro->Radius - std::abs(diff_vector.x);
                        if (dir == LEFT) {
                            shiro->Position.x += penetration; //move ball to right
                            swimShiro->Position.x += penetration;
                        }
                        else {
                            shiro->Position.x -= penetration;    //move ball to left
                            swimShiro->Position.x -= penetration;
                        }
                    }
                    else //vertical collision
                    {
                        shiro->Velocity.y = -shiro->Velocity.y; //reverse vertical velocity
                        swimShiro->Velocity.y = -swimShiro->Velocity.y;

                        //relocate
                        float penetration = shiro->Radius - std::abs(diff_vector.y);
                        if (dir == UP) {
                            shiro->Position.y -= penetration; //move ball back up
                            swimShiro->Position.y -= penetration;
                        }
                        else {
                            shiro->Position.y += penetration; //move ball back down
                            swimShiro->Position.y += penetration;
                        }
                    }
                }
            }
        }
    }
    for (shark& theShark : sharks) {
        if (CheckSharkCollision(*swimShiro, theShark)) {
            // Handle collision between swimShiro and theShark
            //printf("%s", "yess collided");
            --this->Lives;
            if (this->Lives == 0) {
                printf("%s", "lives ghatyo yayy");
                this->ResetLevel();
                this->ResetPlayer();
                this->State = GAME_OVER;
            }
            this->ResetPlayer();
        }
    }

    // also check collisions on PowerUps and if so, activate them
    for (PowerUp& powerUp : this->PowerUps)
    {
        if (!powerUp.Destroyed)
        {
            // first check if powerup passed bottom edge, if so: keep as inactive and destroy
            if (powerUp.Position.y >= this->Height)
                powerUp.Destroyed = true;

            //if (CheckCollision(*Player, powerUp))
            //{	// collided with player, now activate powerup
            //    ActivatePowerUp(powerUp);
            //    powerUp.Destroyed = true;
            //    powerUp.Activated = true;
            //    SoundEngine->play2D("audio/powerup.wav", false);
            //}
        }
    }

    //check collitions for player pad (unless stuck)
    //Collision result = CheckCollision(*Ball, *Player);
    //if (!Ball->Stuck && std::get<0>(result)) {
    //    //check where it hit the board, and change velocity based on where it hit the board
    //    float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
    //    float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
    //    float percentage = distance / (Player->Size.x / 2.0f);

    //    //then move accordingly
    //    float strength = 2.0f;
    //    glm::vec2 oldVelocity = Ball->Velocity;
    //    Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
    //    Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);// keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
    //    Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
    //   
    //    // if Sticky powerup is activated, also stick ball to paddle once new velocity vectors were calculated
    //    Ball->Stuck = Ball->Sticky;
    //    SoundEngine->play2D("audio/bleep.wav", false);
    //}
}

bool CheckCollision(GameObject& one, GameObject& two) { //AABB-AABB collision
    //collision x-axis?
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
        two.Position.x + two.Size.x >= one.Position.x;
    //collision y-axis?
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
        two.Position.y + two.Size.y >= one.Position.y;
    //collision only if on both axes
    return collisionX && collisionY;
}

Collision CheckCollision(PlayerObject& one, GameObject& two) {//AABB - Circle collision
    //get center point circle first
    glm::vec2 center(one.Position + one.Radius);
    //calculate AABB info(center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    //get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    //now that we know the clamped values, add this to AABB_center and we get the value of box closet to circle
    glm::vec2 closest = aabb_center + clamped;
    //now retrieve vector between center of the circle and closest point AABB and check if length<radius
    difference = closest - center;

    if (glm::length(difference) < one.Radius) {//no <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage
        return std::make_tuple(true, VectorDirection(difference), difference);
    }
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

bool CheckSharkCollision(PlayerObject& player, shark& theShark) {
    // Get the center point of the player and the shark
    glm::vec2 playerCenter = player.Position + glm::vec2(player.Radius);
    glm::vec2 sharkCenter = theShark.Position + glm::vec2(theShark.Radius);

    // Calculate the distance between centers
    float distance = glm::length(playerCenter - sharkCenter);

    // Compare distance with the sum of radii
    if (distance < player.Radius + theShark.Radius) {
        return true; // Collided
    }
    else {
        return false; // Not collided
    }
}

//calculate which direction a vector is facing (N,E,S or W)
Direction VectorDirection(glm::vec2 target) {
    glm::vec2 compass[] = {
        glm::vec2(0.0f,1.0f), //up
        glm::vec2(1.0f,0.0f), //right
        glm::vec2(0.0f,-1.0f), //down
        glm::vec2(-1.0f,0.0f)  //left
    };
    float max = 0.0f;
    unsigned int best_match = -1;
    for (unsigned int i = 0; i < 4; i++) {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max) {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}

