//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
}

Renderer::~Renderer()
{
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.aspectRatio = float(m_Width) / float(m_Height);
	m_Camera.Update(pTimer);;
}

void Renderer::Render()
{
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Clear background (black)
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));

	//Define a simple triangle in NDC space
	std::vector<Vertex> vertices_in
	{
		{{ 0.f,  0.5f, 1.f }, {1, 0, 0}}, //Top (Red)
		{{ 0.5f, -0.5f, 1.f }, {0, 1, 0}}, //Bottom Right (Green)
		{{-0.5f, -0.5f, 1.f }, {0, 0, 1}}  //Bottom Left (Blue)
	};

	std::vector<Vertex> vertices_out{};
	VertexTransformationFunction(vertices_in, vertices_out);

	RenderTriangle(vertices_out);

	//Unlock and update window
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.clear();
	vertices_out.reserve(vertices_in.size());

	const Matrix& view = m_Camera.cameraToWorld.Inverse(m_Camera.cameraToWorld);

	for (const auto& v : vertices_in)
	{
		Vertex out = v;

		// Transform to view space
		Vector4 viewPos = view.TransformPoint(Vector4(v.position, 1.f));

		// Perspective divide (simple, no projection yet)
		out.position = { viewPos.x, viewPos.y, viewPos.z };

		// Map to screen space (just visualize Z depth for now)
		out.position.x = (viewPos.x + 1.f) * 0.5f * m_Width;
		out.position.y = (1.f - viewPos.y) * 0.5f * m_Height;

		vertices_out.emplace_back(out);
	}
}

void Renderer::RenderTriangle(const std::vector<Vertex>& vertices) const
{
	if (vertices.size() < 3) return;

	//Extract vertices
	const Vector3& v0 = vertices[0].position;
	const Vector3& v1 = vertices[1].position;
	const Vector3& v2 = vertices[2].position;

	//Bounding box (clamp to screen)
	const int minX = std::max(0, static_cast<int>(std::floor(std::min({ v0.x, v1.x, v2.x }))));
	const int maxX = std::min(m_Width - 1, static_cast<int>(std::ceil(std::max({ v0.x, v1.x, v2.x }))));
	const int minY = std::max(0, static_cast<int>(std::floor(std::min({ v0.y, v1.y, v2.y }))));
	const int maxY = std::min(m_Height - 1, static_cast<int>(std::ceil(std::max({ v0.y, v1.y, v2.y }))));

	//Precompute edge functions
	const Vector2 v0_2D{ v0.x, v0.y };
	const Vector2 v1_2D{ v1.x, v1.y };
	const Vector2 v2_2D{ v2.x, v2.y };

	const float area = Vector2::Cross(v1_2D - v0_2D, v2_2D - v0_2D);
	if (area == 0.f) return;

	//Loop over pixels inside bounding box
	for (int py = minY; py <= maxY; ++py)
	{
		for (int px = minX; px <= maxX; ++px)
		{
			const Vector2 p{ float(px) + 0.5f, float(py) + 0.5f };

			const float w0 = Vector2::Cross(v1_2D - p, v2_2D - p);
			const float w1 = Vector2::Cross(v2_2D - p, v0_2D - p);
			const float w2 = Vector2::Cross(v0_2D - p, v1_2D - p);

			if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
			{
				//Barycentrics normalized
				const float invArea = 1.f / area;
				const float alpha = w0 * invArea;
				const float beta = w1 * invArea;
				const float gamma = w2 * invArea;

				//Interpolate color
				ColorRGB finalColor =
					alpha * vertices[0].color +
					beta * vertices[1].color +
					gamma * vertices[2].color;

				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}