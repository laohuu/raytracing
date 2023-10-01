#include "Renderer.h"

namespace RayTracing
{
    namespace Utils
    {
        static uint32_t ConvertToRGBA(const glm::vec4& color)
        {
            uint8_t r = (uint8_t)(color.r * 255.0f);
            uint8_t g = (uint8_t)(color.g * 255.0f);
            uint8_t b = (uint8_t)(color.b * 255.0f);
            uint8_t a = (uint8_t)(color.a * 255.0f);

            uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
            return result;
        }
    } // namespace Utils

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
        if (m_FinalImage)
        {
            // No resize necessary
            if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
                return;

            m_FinalImage->Resize(width, height);
        }
        else
        {
            m_FinalImage = std::make_shared<Base::Image>(width, height, Base::ImageFormat::RGBA);
        }

        delete[] m_ImageData;
        m_ImageData = new uint32_t[width * height];
    }

    void Renderer::Render(const Scene& scene, const Camera& camera)
    {
        m_ActiveScene  = &scene;
        m_ActiveCamera = &camera;

        for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
        {
            for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
            {
                Ray       ray(camera.GetPosition(), camera.GetRayDirections()[x + y * m_FinalImage->GetWidth()]);
                glm::vec4 color                               = TraceRay(ray);
                color                                         = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
                m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
            }
        }

        m_FinalImage->SetData(m_ImageData);
    }

    glm::vec4 Renderer::TraceRay(const Ray& ray)
    {
        // (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
        // where
        // a = ray origin
        // b = ray direction
        // r = radius
        // t = hit distance

        int   closestSphere = -1;
        float hitDistance   = std::numeric_limits<float>::max();

        glm::vec3 rayOrigin    = ray.GetOrigin();
        glm::vec3 rayDirection = ray.GetDirection();

        for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
        {
            const Sphere& sphere = m_ActiveScene->Spheres[i];
            glm::vec3     origin = rayOrigin - sphere.Position;

            float a = glm::dot(rayDirection, rayDirection);
            float b = 2.0f * glm::dot(origin, rayDirection);

                 // Quadratic forumula discriminant:
                 // b^2 - 4ac
            float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

            float discriminant = b * b - 4.0f * a * c;
            if (discriminant < 0.0f)
                continue;

            // Quadratic formula:
            // (-b +- sqrt(discriminant)) / 2a

            // float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
            float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
            if (closestT > 0.0f && closestT < hitDistance)
            {
                hitDistance   = closestT;
                closestSphere = (int)i;
            }
        }

        if (closestSphere < 0)
            return {0.0f, 0.0f, 0.0f, 1.0f};

        const Sphere& sphere = m_ActiveScene->Spheres[closestSphere];
        glm::vec3     origin = rayOrigin - sphere.Position;

        glm::vec3 hitPoint = origin + rayDirection * hitDistance;
        glm::vec3 normal   = glm::normalize(hitPoint);

        glm::vec3 lightDir       = glm::normalize(glm::vec3(-1, -1, -1));
        float     lightIntensity = glm::max(glm::dot(normal, -lightDir), 0.0f); // == cos(angle)

        glm::vec3 sphereColor = m_ActiveScene->Materials[sphere.MaterialIndex].Albedo;
        sphereColor *= lightIntensity;

        return {sphereColor, 1.0f};
    }
} // namespace RayTracing