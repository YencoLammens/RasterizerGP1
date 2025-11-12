#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera final
	{
		Camera() = default;
		Camera(const Vector3& _origin, float _fovAngle)
			: origin{ _origin }
		{
			ChangeFOV(_fovAngle);
		}

		// --- Camera properties ---
		Vector3 origin{ 0.f, 0.f, 0.f };
		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float fovAngle{ 60.f };
		float fov{};
		float totalPitch{};
		float totalYaw{};

		const float MoveSpeed{ 5.f };
		const float MouseSensitivity{ 2.5f };

		Matrix viewMatrix{};
		Matrix invViewMatrix{};

		// --- Initialization ---
		void Initialize(float _fovAngle = 60.f, Vector3 _origin = { 0.f, 0.f, 0.f })
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			origin = _origin;
			CalculateViewMatrix();
		}

		// --- Per-frame update ---
		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			if (pKeyboardState)
				HandleKeyboardInput(deltaTime, pKeyboardState);

			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);

			HandleMouseInput(deltaTime, mouseState, static_cast<float>(mouseX), static_cast<float>(mouseY));

			// Recalculate view matrix after moving
			CalculateViewMatrix();
		}

		// --- Keyboard controls ---
		void HandleKeyboardInput(float deltaTime, const uint8_t* pKeyboardState)
		{
			if (pKeyboardState[SDL_SCANCODE_W])
				origin += forward * MoveSpeed * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_S])
				origin -= forward * MoveSpeed * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_A])
				origin -= right * MoveSpeed * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_D])
				origin += right * MoveSpeed * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_Q])
				origin -= up * MoveSpeed * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_E])
				origin += up * MoveSpeed * deltaTime;
		}

		// --- Mouse controls ---
		void HandleMouseInput(float deltaTime, uint32_t mouseState, float mouseX, float mouseY)
		{
			// Rotate when holding right mouse button
			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				totalYaw += mouseX * MouseSensitivity * deltaTime * TO_RADIANS;
				totalPitch -= mouseY * MouseSensitivity * deltaTime * TO_RADIANS;

				const float maxPitch = 89.f * TO_RADIANS;
				if (totalPitch > maxPitch) totalPitch = maxPitch;
				if (totalPitch < -maxPitch) totalPitch = -maxPitch;

				const Matrix rotX = Matrix::CreateRotationX(totalPitch);
				const Matrix rotY = Matrix::CreateRotationY(totalYaw);
				const Matrix rot = rotY * rotX;

				forward = rot.TransformVector(Vector3::UnitZ).Normalized();
				right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
				up = Vector3::Cross(forward, right).Normalized();
			}
		}

		// --- Field of view ---
		void ChangeFOV(float _fovAngle)
		{
			fovAngle = _fovAngle;
			fov = tanf((_fovAngle * TO_RADIANS) / 2.f);
		}

		// --- View matrix ---
		void CalculateViewMatrix()
		{
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			viewMatrix = Matrix::CreateLookAtLH(origin, forward, up);
			invViewMatrix = Matrix::Inverse(viewMatrix);
		}
	};
}