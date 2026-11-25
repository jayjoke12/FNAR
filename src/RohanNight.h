#pragma once

#include "Common.h"
#include <raylib.h>
#include <array>
#include <string>
#include <vector>

namespace FNAF {

class Audio;
class AssetManager;

// A separate, asset-free prototype mode for Rohan's house game.
class RohansNightMode {
public:
    void LoadAssets(AssetManager& assets);
    void Start(Audio& audio);
    void HandleInput(Audio& audio);
    void Update(float dt, Audio& audio);
    void Render(bool crtEnabled) const;
    bool ConsumeReturnToMenu();

private:
    enum class Screen { Inactive, Intro, Playing, Paused, Blackout, Jumpscare, GameOver, NightComplete };
    enum class Room { Backyard, Kitchen, LivingRoom, Hallway1, Hallway2, Hallway3, Hallway4,
                      AnjaliRoom, Bathroom, ParentsRoom, D1, W2 };
    enum class Character { Pirate, Skelly };
    enum class Phase { Roaming, EncounterD1, EncounterW2, D1ExtraWait, Returning };

    struct Agent {
        Character character = Character::Pirate;
        Room room = Room::Backyard;
        Room destination = Room::Backyard;
        Phase phase = Phase::Roaming;
        std::vector<Room> route;
        std::vector<Room> history;
        size_t routeIndex = 0;
        int returnIndex = -1;
        float timer = 0.0f;
        float travelDuration = 0.0f;
        bool travelling = false;
        bool defended = false;
    };

    Screen m_screen = Screen::Inactive;
    std::array<Agent, 2> m_agents{};
    int m_d1Owner = -1;
    int m_w2Owner = -1;
    int m_currentCamera = 0;
    int m_jumpscareCharacter = 0;
    float m_introTimer = 0.0f;
    float m_nightTimer = 0.0f;
    float m_power = 100.0f;
    float m_blackoutTimer = 0.0f;
    float m_jumpscareTimer = 0.0f;
    float m_staticTimer = 0.0f;
    float m_powerLogTimer = 0.0f;
    bool m_monitorUp = false;
    bool m_d1Closed = false;
    bool m_d2Closed = false;
    bool m_debugMapVisible = false;
    bool m_debugConsoleVisible = false;
    bool m_returnToMenu = false;
    std::vector<std::string> m_consoleLog;
    Texture2D m_officeBackground{};
    bool m_hasOfficeBackground = false;
    Texture2D m_d1ClosedImage{};
    Texture2D m_d2ClosedImage{};
    Texture2D m_bothDoorsClosedImage{};
    Texture2D m_pirateOfficeImage{};
    Texture2D m_skellyOfficeImage{};
    bool m_hasD1ClosedImage = false;
    bool m_hasD2ClosedImage = false;
    bool m_hasBothDoorsClosedImage = false;
    bool m_hasPirateOfficeImage = false;
    bool m_hasSkellyOfficeImage = false;

    static constexpr float NIGHT_SECONDS = 360.0f;
    static constexpr float ENCOUNTER_SECONDS = 3.0f;

    static float RandomRoomWait();
    static float RandomTravelTime();
    static const char* RoomName(Room room);
    static const char* PhaseName(Phase phase);
    static Vector2 MapPoint(Room room);
    static bool CameraShowsRoom(int camera, Room room);
    static Vector2 VirtualTouchPosition();
    static bool PointIn(Rectangle rect, Vector2 point);
    void ResetNight();
    void SetHouseRoute(Agent& agent);
    void SetD1Route(Agent& agent);
    bool SelectNextRoom(int agentIndex, Room& destination);
    bool BeginTravel(int agentIndex, Room destination);
    void Arrive(int agentIndex, Audio& audio);
    void ResolveTargetBlocked(int agentIndex);
    void StartReturn(int agentIndex);
    void UpdateAgent(int agentIndex, float dt, Audio& audio);
    void TriggerJumpscare(int characterIndex, Audio& audio, bool blackoutFirst = false);
    void AdvanceAfterReturn(int agentIndex);
    void LogEvent(const std::string& message);
    void RenderOffice(bool crtEnabled) const;
    void RenderOfficeCharacter(const Agent& agent, bool atWindow) const;
    void RenderMonitor(bool crtEnabled) const;
    void RenderRoomFeed(Rectangle feed, int camera) const;
    void RenderMap() const;
    void RenderThreatSilhouette(float x, float y, float scale, Character character, bool bright) const;
    void RenderPause() const;
    void RenderDebugMap() const;
    void RenderDebugConsole() const;
    void DrawButton(Rectangle rect, const char* label, bool active = false) const;
};

} // namespace FNAF
