#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include "Math.h"
#include "Timer.h"
#include "Vector3.h"
#include "Matrix.h"
#include "MathHelpers.h"
#include <iostream>
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

		Vector3 origin{};
		Vector3 forward{ -Vector3::UnitZ }; 
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float fovAngle{ 60.f };
		float fovScale{};
		float aspectRatio{ 1.f };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		const float MoveSpeed{ 5.f };
		const float MouseSensitivity{ 25.f };

		Matrix cameraToWorld{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			// Keyboard
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			if (pKeyboardState)
				HandleKeyboardInput(deltaTime, pKeyboardState);

			// Mouse
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			if (mouseState)
				HandleMouseInput(deltaTime, mouseState, float(mouseX), float(mouseY));

			// Update transforms
			CalculateCameraToWorld();
			CalculateViewMatrix();
			CalculateProjectionMatrix();
		}

		Matrix CalculateCameraToWorld()
		{
			// Build orthonormal basis
			right = Vector3::Cross(forward, Vector3::UnitY).Normalized();
			up = Vector3::Cross(right, forward).Normalized();

			cameraToWorld = Matrix(right, up, forward, origin);
			return cameraToWorld;
		}

		void CalculateViewMatrix()
		{
			right = Vector3::Cross(forward, Vector3::UnitY).Normalized();
			up = Vector3::Cross(right, forward).Normalized();

			cameraToWorld = Matrix(right, up, forward, origin);

			viewMatrix = Matrix::Inverse(cameraToWorld);

			//viewMatrix = Matrix::CreateLookAtLH(origin, forward, Vector3::UnitY);
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(
				fovAngle * TO_RADIANS,
				aspectRatio,
				0.1f,
				1000.f
			);
		}

		void HandleKeyboardInput(const float& deltaTime, const uint8_t* pKeyboardState)
		{
			// Movement relative to camera basis
			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
				origin += forward * deltaTime * MoveSpeed;
			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
				origin -= forward * deltaTime * MoveSpeed;
			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
				origin -= right * deltaTime * MoveSpeed;
			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
				origin += right * deltaTime * MoveSpeed;
		}

		void HandleMouseInput(const float& deltaTime, const uint32_t& mouseState, float mouseX, float mouseY)
		{
			// Rotate (RMB)
			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT) && !(mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)))
			{
				totalYaw += mouseX * MouseSensitivity * deltaTime * TO_RADIANS;
				totalPitch += -mouseY * MouseSensitivity * deltaTime * TO_RADIANS;

				Matrix rotX = Matrix::CreateRotationX(totalPitch);
				Matrix rotY = Matrix::CreateRotationY(totalYaw);
				Matrix rot = rotY * rotX;

				forward = rot.TransformVector(-Vector3::UnitZ).Normalized(); // forward is -Z

			}
			// Dolly (LMB)
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && !(mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)))
			{
				origin += -forward * deltaTime * MouseSensitivity * mouseY;
				totalYaw += mouseX * MouseSensitivity * deltaTime * TO_RADIANS;

				Matrix rotY = Matrix::CreateRotationY(totalYaw);
				forward = rotY.TransformVector(-Vector3::UnitZ).Normalized();
			}
			// Pan (MMB or LMB+RMB)
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE) ||
				(mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)))
			{
				origin += -up * deltaTime * MouseSensitivity * mouseY;
				origin += right * deltaTime * MouseSensitivity * mouseX;
			}
		}

		void ChangeFOV(float _fovAngle)
		{
			fovAngle = _fovAngle;
			fovScale = tanf((_fovAngle * TO_RADIANS) / 2.f);
			std::cout << fovAngle << " degrees -> fovScale: " << fovScale << std::endl;
		}
	};
}