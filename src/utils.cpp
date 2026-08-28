#include "utils.hpp"
#include <atomic>
#include <cstdint>
#include <random>
#include <sstream>
#include <iomanip>

namespace raphEngine
{
    glm::vec3 Utils::GetDirectionFromRotation(const glm::vec3& rotation)
    {
        float pitch = glm::radians(rotation.x);
        float yaw = glm::radians(rotation.y);

        glm::vec3 direction;
        direction.x = cos(pitch) * sin(yaw);
        direction.y = -sin(pitch);
        direction.z = -cos(pitch) * cos(yaw);

        return glm::normalize(direction);
    }

    glm::vec3 Utils::GetForwardFromModelMatrix(const glm::mat4& model)
    {
        return glm::normalize(glm::vec3(model[1]));
    }

    glm::vec3 Utils::GetForwardFromRotation(const glm::vec3& rotationDegrees)
    {
        glm::mat4 rot = glm::toMat4(glm::quat(glm::radians(rotationDegrees)));
        return glm::normalize(glm::vec3(rot[1]));
    }

    long Utils::get_id()
    {
        static std::atomic<long> counter{ 0 };
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    std::string Utils::get_uuid()
    {
        static thread_local std::mt19937_64 engine{ std::random_device{}() };
        static thread_local std::uniform_int_distribution<uint64_t> dist;

        uint64_t hi = dist(engine);
        uint64_t lo = dist(engine);

        hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(8)
            << static_cast<uint32_t>(hi >> 32) << '-' << std::setw(4)
            << static_cast<uint16_t>(hi >> 16) << '-' << std::setw(4)
            << static_cast<uint16_t>(hi) << '-' << std::setw(4)
            << static_cast<uint16_t>(lo >> 48) << '-' << std::setw(12)
            << (lo & 0xFFFFFFFFFFFFULL);

        return oss.str();
    }

    bool Utils::compare_uuid(const std::string& lhs, const std::string& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        return std::equal(
            lhs.begin(), lhs.end(), rhs.begin(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a))
                    == std::tolower(static_cast<unsigned char>(b));
            });
    }
} // namespace raphEngine
