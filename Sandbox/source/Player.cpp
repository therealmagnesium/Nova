#include "Player.h"
#include "Game.h"

using namespace Nova;

static constexpr float k_AnimationCrossfadeDuration = 0.2f;

void Update_Idle(Scene& scene, Player& player, const glm::vec3& move_direction);
void Update_Run(Scene& scene, Player& player, const glm::vec3& move_direction);
void Update_Jump(Scene& scene, Player& player, const glm::vec3& move_direction);
void SwitchToState(Scene& scene, Player& player, u8 new_state);
void PlayAnimationForState(Scene& scene, Player& player, u8 state);
void ApplyGravity(Player& player, TransformComponent& transform);
void FaceMoveDirection(Player& player, TransformComponent& transform, const glm::vec3& move_direction, float delta_time);
glm::vec3 GetCameraRelativeMoveDirection();

Player Player_Create(Scene& scene)
{
    Player player;

    const AssetHandle asset_robot = Game::GetAsset(Assets::ModelRobot);

    AssetHandle assets_animations[3];
    assets_animations[0] = Game::GetAsset(Assets::AnimationIdle);
    assets_animations[1] = Game::GetAsset(Assets::AnimationRun);
    assets_animations[2] = Game::GetAsset(Assets::AnimationJump);

    player.entity = Scenes::CreateEntity(scene, "Player");
    player.entity.AddComponent<AnimatorComponent>(scene, asset_robot, assets_animations, LEN(assets_animations));
    player.entity.GetComponent<TransformComponent>(scene)->position = glm::vec3(0.f, -1.f, 0.f);

    return player;
}

void Player_Update(Scene& scene, Player& player)
{
    if (scene.state != SceneState::Runtime)
        return;

    const auto transform = player.entity.GetComponent<TransformComponent>(scene);
    if (transform == NULL)
        return;

    const glm::vec3 move_direction = GetCameraRelativeMoveDirection();

    switch (player.state)
    {
        case PlayerState::Idle:
            Update_Idle(scene, player, move_direction);
            break;
        case PlayerState::Run:
            Update_Run(scene, player, move_direction);
            break;
        case PlayerState::Jump:
            Update_Jump(scene, player, move_direction);
            break;

        default:
            break;
    }

    FaceMoveDirection(player, *transform, move_direction, Application::GetDeltaTime());
}

void Update_Idle(Scene& scene, Player& player, const glm::vec3& move_direction)
{
    const auto transform = player.entity.GetComponent<TransformComponent>(scene);
    ApplyGravity(player, *transform); // Keeps the player glued to the ground / catches it falling off a ledge later

    if (Input::IsKeyPressed(KEY_SPACE) && player.is_grounded)
    {
        player.velocity.y = player.jump_speed;
        player.is_grounded = false;
        SwitchToState(scene, player, PlayerState::Jump);
        return;
    }

    if (glm::dot(move_direction, move_direction) > 0.001f)
        SwitchToState(scene, player, PlayerState::Run);
}

void Update_Run(Scene& scene, Player& player, const glm::vec3& move_direction)
{
    const auto transform = player.entity.GetComponent<TransformComponent>(scene);
    ApplyGravity(player, *transform);

    if (Input::IsKeyPressed(KEY_SPACE) && player.is_grounded)
    {
        player.velocity.y = player.jump_speed;
        player.is_grounded = false;
        SwitchToState(scene, player, PlayerState::Jump);
        return;
    }

    if (glm::dot(move_direction, move_direction) <= 0.001f)
    {
        SwitchToState(scene, player, PlayerState::Idle);
        return;
    }

    transform->position += move_direction * player.move_speed * Application::GetDeltaTime();
}

void Update_Jump(Scene& scene, Player& player, const glm::vec3& move_direction)
{
    const auto transform = player.entity.GetComponent<TransformComponent>(scene);

    // Limited air control - same direction as ground movement, at reduced authority
    transform->position += move_direction * player.move_speed * 0.5f * Application::GetDeltaTime();
    ApplyGravity(player, *transform);

    if (player.is_grounded)
        SwitchToState(scene, player, glm::dot(move_direction, move_direction) > 0.001f ? PlayerState::Run : PlayerState::Idle);
}

void SwitchToState(Scene& scene, Player& player, u8 new_state)
{
    if (player.state == new_state)
        return;

    player.state = new_state;
    PlayAnimationForState(scene, player, new_state);
}

void PlayAnimationForState(Scene& scene, Player& player, u8 state)
{
    const auto ac = player.entity.GetComponent<AnimatorComponent>(scene);

    u8 clip_index = Assets::AnimationIdle;
    bool loop = true;

    switch (state)
    {
        case PlayerState::Idle:
            clip_index = Assets::AnimationIdle;
            loop = true;
            break;
        case PlayerState::Run:
            clip_index = Assets::AnimationRun;
            loop = true;
            break;
        case PlayerState::Jump:
            clip_index = Assets::AnimationJump;
            loop = false; // Plays once over the jump arc rather than looping mid-air
            break;
    }

    const AnimationClip* clip = AssetManager::GetAsset<AnimationClip>(Game::GetAsset(clip_index));
    if (clip == NULL)
        return;

    Animators::CrossfadeTo(ac->animator, *clip, k_AnimationCrossfadeDuration, loop);
}

void ApplyGravity(Player& player, TransformComponent& transform)
{
    const float delta_time = Application::GetDeltaTime();

    player.velocity.y += player.gravity * delta_time;
    transform.position.y += player.velocity.y * delta_time;

    if (transform.position.y <= player.ground_height)
    {
        transform.position.y = player.ground_height;
        player.velocity.y = 0.f;
        player.is_grounded = true;
    }
    else
    {
        player.is_grounded = false;
    }
}

void FaceMoveDirection(Player& player, TransformComponent& transform, const glm::vec3& move_direction, float delta_time)
{
    if (glm::dot(move_direction, move_direction) <= 0.001f)
        return; // No input - hold the last facing direction rather than snapping to a default

    const float target_yaw = glm::degrees(atan2f(move_direction.x, move_direction.z));

    // Shortest-path angular delta - avoids spinning the "long way" around when crossing the
    // -180/180 wrap (e.g. turning from -179 to 179 degrees).
    float yaw_delta = target_yaw - transform.rotation.y;
    yaw_delta = glm::mod(yaw_delta + 180.f, 360.f) - 180.f;

    // Exponential (frame-rate independent) smoothing rather than a constant-speed clamp. This is
    // what actually removes the "snap": a fixed degrees/second turn rate still starts and stops
    // turning abruptly (see Section 3.1), which reads as snappy for the small heading changes
    // WASD produces most of the time. Closing a fraction of the remaining angle each frame eases
    // in AND out smoothly, and can never overshoot since it only ever closes PART of the gap -
    // 1 - exp(-rate * dt) is the fraction closed this frame, and it's the same fraction
    // regardless of frame rate, unlike a fixed-factor lerp.
    const float t = 1.f - expf(-player.turn_smoothing * delta_time);
    transform.rotation.y += yaw_delta * t;
}

glm::vec3 GetCameraRelativeMoveDirection()
{
    const float input_x = Input::GetAxis(InputAxis::Horizontal) + Input::GetAxisAlt(InputAxis::Horizontal);
    const float input_z = Input::GetAxis(InputAxis::Vertical) - Input::GetAxisAlt(InputAxis::Vertical);

    if (fabsf(input_x) < 0.001f && fabsf(input_z) < 0.001f)
        return glm::vec3(0.f);

    const Camera3D* camera = Renderer::GetPrimaryCamera();
    if (camera == NULL)
        return glm::vec3(0.f);

    constexpr glm::vec3 k_WorldUp = glm::vec3(0.f, 1.f, 0.f);

    glm::vec3 forward = camera->target - camera->position;
    forward.y = 0.f;
    if (glm::dot(forward, forward) < 0.0001f)
        forward = glm::vec3(0.f, 0.f, 1.f);
    forward = glm::normalize(forward);

    const glm::vec3 right = glm::normalize(glm::cross(forward, k_WorldUp));
    const glm::vec3 direction = forward * input_z + right * input_x;

    return glm::dot(direction, direction) > 0.0001f ? glm::normalize(direction) : glm::vec3(0.f);
}
