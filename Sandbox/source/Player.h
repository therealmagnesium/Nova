#pragma once
#include <Nova.h>
#include <glm/vec3.hpp>

using namespace Nova;

namespace PlayerState
{
    enum : u8
    {
        Idle,
        Run,
        Jump,
        _Length
    };
}

struct Player
{
    Entity entity;
    u8 state = PlayerState::Idle;

    glm::vec3 velocity = glm::vec3(0.f);
    float move_speed = 4.f;      // Units/second while running
    float turn_smoothing = 15.f; // Higher = snappier turning, lower = smoother/slower (see FaceMoveDirection)
    float jump_speed = 9.f;      // Initial upward velocity applied on jump
    float gravity = -18.f;       // Downward acceleration applied every frame while airborne
    float ground_height = -1.f;  // World Y considered "grounded"
    bool is_grounded = true;
};

Player Player_Create(Scene& scene);
void Player_Update(Scene& scene, Player& player);
