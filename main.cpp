#include "memory.h"
#include "vector.h"

#include <thread>

namespace offset
{
    // Roblox offsets from your list
    constexpr ::std::ptrdiff_t RenderToEngine = 0x10;
    constexpr ::std::ptrdiff_t RenderToFakeDataModel = 0x120;
    constexpr ::std::ptrdiff_t FakeDataModelToRealDataModel = 0x1b8;
    constexpr ::std::ptrdiff_t Name = 0x70;
    constexpr ::std::ptrdiff_t Children = 0x78;
    constexpr ::std::ptrdiff_t Parent = 0x50;
    constexpr ::std::ptrdiff_t ChildSize = 0x8;
    constexpr ::std::ptrdiff_t LocalPlayer = 0x120;
    constexpr ::std::ptrdiff_t ModelInstance = 0x2e0;
    constexpr ::std::ptrdiff_t Team = 0x210;
    constexpr ::std::ptrdiff_t Health = 0x194;
    constexpr ::std::ptrdiff_t BasePartPosition = 0x140;
    constexpr ::std::ptrdiff_t CFrame = 0x11c;
    constexpr ::std::ptrdiff_t CurrentCamera = 0x3f0;
    constexpr ::std::ptrdiff_t CameraPosition = 0x104;
    constexpr ::std::ptrdiff_t CameraRotation = 0xe0;
}

Vector3 CalculateAngle(
    const Vector3& localPosition,
    const Vector3& enemyPosition,
    const Vector3& viewAngles) noexcept
{
    return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

int main()
{
    // Initialize memory class with Roblox process
    const auto memory = Memory("RobloxPlayerBeta.exe");

    // Get the Roblox module address
    const auto robloxBase = memory.GetModuleAddress("RobloxPlayerBeta.exe");

    if (!robloxBase) {
        return 1; // Exit if Roblox isn't running
    }

    // Infinite hack loop
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Aimbot key
        if (!GetAsyncKeyState(VK_RBUTTON))
            continue;

        // Get the render address - this is an example method, you may need to adjust
        // This assumes there's a static address pointing to the render object
        const auto renderAddress = memory.Read<std::uintptr_t>(robloxBase + 0x2000000); // Example offset

        if (!renderAddress)
            continue;

        // Navigate to data model
        const auto fakeDataModel = memory.Read<std::uintptr_t>(renderAddress + offset::RenderToFakeDataModel);
        const auto dataModel = memory.Read<std::uintptr_t>(fakeDataModel + offset::FakeDataModelToRealDataModel);

        if (!dataModel)
            continue;

        // Get local player
        const auto localPlayer = memory.Read<std::uintptr_t>(dataModel + offset::LocalPlayer);

        if (!localPlayer)
            continue;

        // Get local player model and team
        const auto localModel = memory.Read<std::uintptr_t>(localPlayer + offset::ModelInstance);
        const auto localTeam = memory.Read<std::int32_t>(localPlayer + offset::Team);

        // Get head position
        const auto localHead = memory.Read<std::uintptr_t>(localModel + 0x150); // Example offset to head
        const auto localPosition = memory.Read<Vector3>(localHead + offset::BasePartPosition);

        // Get camera
        const auto camera = memory.Read<std::uintptr_t>(dataModel + offset::CurrentCamera);
        const auto cameraPosition = memory.Read<Vector3>(camera + offset::CameraPosition);
        const auto cameraRotation = memory.Read<Vector3>(camera + offset::CameraRotation);

        // Aimbot FOV
        auto bestFov = 5.f;
        auto bestAngle = Vector3{ };

        // Get workspace
        const auto workspace = memory.Read<std::uintptr_t>(dataModel + 0x28); // Example offset to workspace

        // Get workspace children (players)
        const auto childrenPtr = memory.Read<std::uintptr_t>(workspace + offset::Children);
        const auto childrenCount = memory.Read<int>(childrenPtr + 0x4); // Assuming size is at offset +4

        for (int i = 0; i < childrenCount; i++)
        {
            const auto childPtr = memory.Read<std::uintptr_t>(childrenPtr + 0x8 + i * offset::ChildSize);

            if (!childPtr)
                continue;

            // Check if it's a player - this needs to be refined based on Roblox's structure
            // This is just a placeholder approach
            const auto childName = memory.Read<std::uintptr_t>(childPtr + offset::Name);

            // Get player attributes
            const auto childTeam = memory.Read<std::int32_t>(childPtr + offset::Team);
            const auto childHealth = memory.Read<float>(childPtr + offset::Health);

            // Skip players on same team or dead players
            if (childTeam == localTeam || childHealth <= 0)
                continue;

            // Get player's model
            const auto childModel = memory.Read<std::uintptr_t>(childPtr + offset::ModelInstance);

            if (!childModel)
                continue;

            // Get head part
            const auto head = memory.Read<std::uintptr_t>(childModel + 0x150); // Example offset to head

            if (!head)
                continue;

            const auto headPosition = memory.Read<Vector3>(head + offset::BasePartPosition);

            // Calculate angle for aimbot
            const auto angle = CalculateAngle(
                cameraPosition,
                headPosition,
                cameraRotation
            );

            const auto fov = std::hypot(angle.x, angle.y);

            if (fov < bestFov)
            {
                bestFov = fov;
                bestAngle = angle;
            }
        }

        // If we have a best angle, do aimbot
        if (!bestAngle.IsZero())
        {
            // Write to camera rotation
            memory.Write<Vector3>(camera + offset::CameraRotation, cameraRotation + bestAngle / 3.f); // Smoothing
        }
    }

    return 0;
}