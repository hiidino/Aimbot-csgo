#include "memory.h"
#include "vector.h"
#include <Windows.h>
#include <thread>
#include <cstdint>

namespace offset {
	// Client
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDA746C;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4DC178C;
	
	// Engine
	constexpr::std::ptrdiff_t dwClientState = 0x589FCC;
	constexpr::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;

	// Entity 
	constexpr::std::ptrdiff_t m_dwBoneMatrix = 0x26AB;
	constexpr::std::ptrdiff_t m_bDormant = 0xED;
	constexpr::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr::std::ptrdiff_t m_iHealth = 0x100;
	constexpr::std::ptrdiff_t m_vecOrigin = 0x138;
	constexpr::std::ptrdiff_t m_vecViewOffset = 0x100;
	constexpr::std::ptrdiff_t m_aimPunchAngle = 0x303C;
	constexpr::std::ptrdiff_t m_bSpottedByMask = 0x900;
 }

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles
) noexcept {
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

int main() {
		
	// Initalise Memory class
	const auto memory = Memory{ "csgo.exe" };

	// Module addresses 
	const auto client = memory.GetModuleAddress("client.dll");
	const auto engine = memory.GetModuleAddress("engine.dll");

	// hack loop 
	while (true) {

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		// Aimbot key 
		if (!GetAsyncKeyState(VK_RBUTTON)) {
			continue;
		}

		// get local player 
		const auto& localPlayer = memory.Read<std::uintptr_t>(client + offset::dwLocalPlayer);

		// get local team 
		const auto& localTeam = memory.Read<std::int32_t>(localPlayer + offset::m_iTeamNum);

		// eye position = origin + viewOffset
		const auto localEyePosition = memory.Read<Vector3>(localPlayer + offset::m_vecOrigin) + memory.Read<Vector3>(localPlayer + offset::m_vecViewOffset);

		// Client State pointer
		const auto& clientState = memory.Read<std::uintptr_t>(engine + offset::dwClientState);

		// View angles pointer
		const auto& viewAngles = memory.Read<Vector3>(clientState + offset::dwClientState_ViewAngles);

		// Aim punch 
		const auto& aimPunch = memory.Read<Vector3>(localPlayer + offset::m_aimPunchAngle) * 2;

		auto bestFov = 5.f;
		auto bestAngle = Vector3{ };

		for (auto i = 1; i <= 32; ++i){
			const auto& player = memory.Read<std::uintptr_t>(client + offset::dwEntityList + (i * 0x10));

			// entity checks
			if (memory.Read<std::int32_t>(player + offset::m_iTeamNum) == localTeam) {
				continue;
			} 

			if (memory.Read<bool>(player + offset::m_bDormant)) {
				continue;
			}

			if (!memory.Read<std::int32_t>(player + offset::m_iHealth)) {
				continue;
			}

			if (!memory.Read<bool>(player + offset::m_bSpottedByMask)) {
				continue;
			}

			const auto boneMatrix = memory.Read<std::uintptr_t>(player + offset::m_dwBoneMatrix);

			// position of player head in 3D space 
			const auto playerHeadPosition = Vector3{
				memory.Read<float>(boneMatrix + 0x30 * 8 + 0x0C),
				memory.Read<float>(boneMatrix + 0x30 * 8 + 0x1C),
				memory.Read<float>(boneMatrix + 0x30 * 8 + 0x2C)
			};

			const auto angle = CalculateAngle(
				localEyePosition,
				playerHeadPosition,
				viewAngles + aimPunch
			);

			const auto fov = std::hypot(angle.x, angle.y);

			if (fov < bestFov) {
				bestFov = fov;
				bestAngle = angle;
			}
		}

		// if we have a bestangle, trigger aimbot
		if (!bestAngle.IsZero()){
			memory.Write<Vector3>(clientState + offset::dwClientState_ViewAngles, viewAngles + bestAngle / 3.f);
		}
	}
		7
	return 0;
}