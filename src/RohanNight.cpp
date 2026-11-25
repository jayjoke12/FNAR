#include "RohanNight.h"
#include "Audio.h"
#include "AssetManager.h"
#include <algorithm>
#include <cmath>

namespace FNAF {
namespace {
constexpr Rectangle kPause{1190, 30, 58, 48};
constexpr Rectangle kResume{465, 370, 350, 58};
constexpr Rectangle kPauseMenu{465, 445, 350, 58};
constexpr Rectangle kMonitor{520, 620, 240, 72};
constexpr Rectangle kD1{35, 620, 210, 72};
// Jay chose D2 as the prototype defense for W2; this remains an explicit mapping.
constexpr Rectangle kD2{1035, 620, 210, 72};
constexpr Rectangle kRetry{465, 435, 350, 58};
constexpr Rectangle kMenu{465, 510, 350, 58};
constexpr std::array<Rectangle, 5> kCamButtons{{
    {110,616,190,70}, {320,616,190,70}, {530,616,190,70}, {740,616,190,70}, {950,616,190,70}
}};
constexpr Color kPale{205,216,225,255};
constexpr Color kCyan{65,205,220,255};
constexpr Color kOrange{232,133,72,255};
}

void RohansNightMode::LoadAssets(AssetManager& assets) {
    auto loadOptional = [&assets](const char* path, Texture2D& texture, bool& loaded) {
        if (!FileExists(path)) { texture = Texture2D{}; loaded = false; return; }
        texture = assets.GetTexture(path);
        loaded = texture.id != 0 && texture.width > 1 && texture.height > 1;
    };
    loadOptional("assets/rohans_night/images/office.png", m_officeBackground, m_hasOfficeBackground);
    loadOptional("assets/rohans_night/images/office_doorLclosed.png", m_d1ClosedImage, m_hasD1ClosedImage);
    loadOptional("assets/rohans_night/images/office_doorRclosed.png", m_d2ClosedImage, m_hasD2ClosedImage);
    loadOptional("assets/rohans_night/images/office_doorBothClosed.png", m_bothDoorsClosedImage, m_hasBothDoorsClosedImage);
    loadOptional("assets/rohans_night/images/pirate_office.png", m_pirateOfficeImage, m_hasPirateOfficeImage);
    loadOptional("assets/rohans_night/images/skelly_office.png", m_skellyOfficeImage, m_hasSkellyOfficeImage);
}

float RohansNightMode::RandomRoomWait() { return static_cast<float>(GetRandomValue(20,70))/10.0f; }
float RohansNightMode::RandomTravelTime() { return static_cast<float>(GetRandomValue(13,50))/10.0f; }
bool RohansNightMode::PointIn(Rectangle r, Vector2 p) { return CheckCollisionPointRec(p,r); }

const char* RohansNightMode::PhaseName(Phase p) {
    switch (p) {
        case Phase::Roaming: return "roaming / waiting";
        case Phase::EncounterD1: return "D1 encounter";
        case Phase::EncounterW2: return "W2 encounter";
        case Phase::D1ExtraWait: return "D1 blocked / extra wait";
        case Phase::Returning: return "returning along route";
    }
    return "unknown";
}

Vector2 RohansNightMode::MapPoint(Room r) {
    switch (r) {
        case Room::Backyard: return {180,165}; case Room::Kitchen: return {195,290};
        case Room::LivingRoom: return {185,515}; case Room::Hallway1: return {395,420};
        case Room::Hallway2: return {660,490}; case Room::Hallway3: return {750,378};
        case Room::Hallway4: return {720,305}; case Room::AnjaliRoom: return {735,180};
        case Room::Bathroom: return {1010,180}; case Room::ParentsRoom: return {615,610};
        case Room::D1: return {930,490}; case Room::W2: return {1090,390};
    }
    return {0,0};
}

void RohansNightMode::LogEvent(const std::string& message) {
    const int minute=static_cast<int>(m_nightTimer/60.0f);
    const int second=static_cast<int>(m_nightTimer)%60;
    m_consoleLog.push_back(TextFormat("[%02d:%02d] %s",minute,second,message.c_str()));
    while(m_consoleLog.size()>12) m_consoleLog.erase(m_consoleLog.begin());
}

const char* RohansNightMode::RoomName(Room r) {
    switch (r) {
        case Room::Backyard: return "BACKYARD"; case Room::Kitchen: return "KITCHEN";
        case Room::LivingRoom: return "LIVING ROOM"; case Room::Hallway1: return "HALLWAY 1";
        case Room::Hallway2: return "HALLWAY 2"; case Room::Hallway3: return "HALLWAY 3";
        case Room::Hallway4: return "HALLWAY 4"; case Room::AnjaliRoom: return "ANJALI'S ROOM";
        case Room::Bathroom: return "BATHROOM"; case Room::ParentsRoom: return "PARENTS ROOM";
        case Room::D1: return "D1 / HALL APPROACH"; case Room::W2: return "W2 / BATHROOM WINDOW";
    }
    return "UNKNOWN ROOM";
}

bool RohansNightMode::CameraShowsRoom(int camera, Room r) {
    switch (camera) {
        case 0: return r == Room::LivingRoom;
        case 1: return r == Room::Kitchen;
        case 2: return r == Room::AnjaliRoom;
        case 3: return r == Room::Hallway1 || r == Room::Hallway2;
        case 4: return r == Room::ParentsRoom;
        default: return false;
    }
}

Vector2 RohansNightMode::VirtualTouchPosition() {
    const Vector2 p = GetTouchPosition(0);
    const GameViewport v = GetGameViewport();
    return {(p.x-v.offsetX)/v.scale,(p.y-v.offsetY)/v.scale};
}

void RohansNightMode::Start(Audio& audio) {
    m_returnToMenu = false;
    ResetNight();
    m_screen = Screen::Intro;
    audio.PlayMenuMusic(false);
    audio.PlayAmbient(true);
    audio.PlayFanHum(true);
}

void RohansNightMode::ResetNight() {
    m_agents = {};
    m_agents[0].character = Character::Pirate;
    m_agents[0].room = Room::Backyard;
    m_agents[0].history = {Room::Backyard};
    m_agents[0].timer = RandomRoomWait();
    m_agents[1].character = Character::Skelly;
    m_agents[1].room = Room::LivingRoom;
    m_agents[1].history = {Room::LivingRoom};
    m_agents[1].timer = RandomRoomWait();
    m_d1Owner = m_w2Owner = -1;
    m_currentCamera = 0;
    m_introTimer = m_nightTimer = m_blackoutTimer = m_jumpscareTimer = m_staticTimer = 0.0f;
    m_power = 100.0f;
    m_monitorUp = m_d1Closed = m_d2Closed = false;
    m_powerLogTimer=0.0f;
    m_debugMapVisible=m_debugConsoleVisible=false;
    m_consoleLog.clear();
    LogEvent("Night initialized; Pirate at Backyard, Skelly at Living Room.");
}

void RohansNightMode::HandleInput(Audio& audio) {
    Vector2 pointer = GetVirtualMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
#if defined(__ANDROID__)
    static bool wasDown = false;
    const bool down = GetTouchPointCount() > 0;
    if (down && !wasDown) { clicked = true; pointer = VirtualTouchPosition(); }
    wasDown = down;
#endif
    if (m_screen == Screen::Inactive) return;
    if (m_screen == Screen::Intro) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) m_introTimer = 3.0f;
        return;
    }
    if (m_screen == Screen::Playing) {
        if (IsKeyPressed(KEY_ESCAPE) || (clicked && PointIn(kPause,pointer))) { m_screen=Screen::Paused; return; }
        if (IsKeyPressed(KEY_F1)) m_debugMapVisible=!m_debugMapVisible;
        if (IsKeyPressed(KEY_F3)) m_debugConsoleVisible=!m_debugConsoleVisible;
        if (IsKeyPressed(KEY_F2)) m_agents[0].timer=0.0f;
        if (IsKeyPressed(KEY_F4)) m_agents[1].timer=0.0f;
        if (IsKeyPressed(KEY_SPACE) || (clicked && PointIn(kMonitor,pointer))) {
            m_monitorUp=!m_monitorUp;
            LogEvent(std::string("Player ")+(m_monitorUp?"raised":"lowered")+" the monitor.");
            audio.PlayCameraFlip();
        }
        if (m_monitorUp) {
            int selected=-1;
            if (IsKeyPressed(KEY_ONE)) selected=0;
            if (IsKeyPressed(KEY_TWO)) selected=1;
            if (IsKeyPressed(KEY_THREE)) selected=2;
            if (IsKeyPressed(KEY_FOUR)) selected=3;
            if (IsKeyPressed(KEY_FIVE)) selected=4;
            if (selected>=0 && selected!=m_currentCamera) {
                m_currentCamera=selected;
                LogEvent(std::string("Monitor switched to C")+std::to_string(selected+1)+".");
                audio.PlayCameraSwitch();
            }
            for (int i=0;i<5;++i) if (clicked && PointIn(kCamButtons[static_cast<size_t>(i)],pointer)) {
                if (m_currentCamera!=i) {
                    m_currentCamera=i;
                    LogEvent(std::string("Monitor switched to C")+std::to_string(i+1)+".");
                    audio.PlayCameraSwitch();
                }
            }
        } else {
            if (IsKeyPressed(KEY_Q) || (clicked && PointIn(kD1,pointer))) {
                m_d1Closed=!m_d1Closed;
                if (m_d1Closed) for (Agent& a : m_agents) if (a.phase==Phase::EncounterD1) a.defended=true;
                LogEvent(std::string("Player ")+(m_d1Closed?"closed":"opened")+
                         " D1; power load "+(m_d1Closed?"increases":"decreases")+" by 0.08%/second.");
                audio.PlayDoorSound(m_d1Closed);
            }
            if (IsKeyPressed(KEY_E) || (clicked && PointIn(kD2,pointer))) {
                m_d2Closed=!m_d2Closed;
                if (m_d2Closed) for (Agent& a : m_agents) if (a.phase==Phase::EncounterW2) a.defended=true;
                LogEvent(std::string("Player ")+(m_d2Closed?"closed":"opened")+
                         " D2 (W2 defense); power load "+(m_d2Closed?"increases":"decreases")+" by 0.08%/second.");
                audio.PlayDoorSound(m_d2Closed);
            }
        }
        return;
    }
    if (m_screen == Screen::Paused) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
            (clicked && (PointIn(kPause,pointer)||PointIn(kResume,pointer)))) m_screen=Screen::Playing;
        else if (clicked && PointIn(kPauseMenu,pointer)) { m_returnToMenu=true; m_screen=Screen::Inactive; audio.StopAllSounds(); }
        return;
    }
    if (m_screen == Screen::GameOver || m_screen == Screen::NightComplete) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M) || (clicked && PointIn(kMenu,pointer))) {
            m_returnToMenu=true; m_screen=Screen::Inactive; audio.StopAllSounds();
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R) || (clicked && PointIn(kRetry,pointer))) {
            ResetNight(); m_screen=Screen::Playing; audio.PlayFanHum(true);
        }
    }
}

void RohansNightMode::SetHouseRoute(Agent& a) {
    a.route={Room::Hallway3,Room::Hallway4,Room::AnjaliRoom,Room::Bathroom,Room::W2}; a.routeIndex=0;
}
void RohansNightMode::SetD1Route(Agent& a) { a.route={Room::D1}; a.routeIndex=0; }

bool RohansNightMode::SelectNextRoom(int i, Room& dest) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    if (a.routeIndex<a.route.size()) { dest=a.route[a.routeIndex++]; return true; }
    if (a.character==Character::Pirate) {
        if (a.room==Room::Backyard) dest=Room::Kitchen;
        else if (a.room==Room::Kitchen) dest=Room::Hallway1;
        else if (a.room==Room::Hallway1) dest=Room::Hallway2;
        else if (a.room==Room::Hallway2) {
            // Route odds are unspecified in the brief; the prototype uses an even split.
            if (GetRandomValue(0,1)==0) { SetD1Route(a); LogEvent("Pirate route roll at Hallway 2: D1."); }
            else { SetHouseRoute(a); LogEvent("Pirate route roll at Hallway 2: house route via Hallway 3."); }
            return SelectNextRoom(i,dest);
        } else if (a.room==Room::Bathroom) dest=Room::W2;
        else return false;
        return true;
    }
    if (a.room==Room::LivingRoom) dest=Room::Hallway1;
    else if (a.room==Room::Hallway1) dest=Room::Hallway2;
    else if (a.room==Room::Hallway2) {
        const int branch=GetRandomValue(0,2);
        if (branch==0) { a.route={Room::ParentsRoom}; a.routeIndex=0; LogEvent("Skelly route roll at Hallway 2: Parents Room pause."); }
        else if (branch==1) { SetD1Route(a); LogEvent("Skelly route roll at Hallway 2: D1."); }
        else { SetHouseRoute(a); LogEvent("Skelly route roll at Hallway 2: house route via Hallway 3."); }
        return SelectNextRoom(i,dest);
    } else if (a.room==Room::ParentsRoom) {
        if (GetRandomValue(0,1)==0) { SetD1Route(a); LogEvent("Skelly left Parents Room after 4 seconds; chose D1."); }
        else { SetHouseRoute(a); LogEvent("Skelly left Parents Room after 4 seconds; chose house route."); }
        return SelectNextRoom(i,dest);
    } else if (a.room==Room::Bathroom) dest=Room::W2;
    else return false;
    return true;
}

void RohansNightMode::ResolveTargetBlocked(int i) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+
             " found "+RoomName(a.destination)+" occupied; rerouting through Hallway 3.");
    if (a.destination==Room::D1 && m_d1Owner==i) m_d1Owner=-1;
    if (a.destination==Room::W2 && m_w2Owner==i) m_w2Owner=-1;
    a.travelling=false; a.route.clear(); SetHouseRoute(a); a.timer=RandomRoomWait();
}

bool RohansNightMode::BeginTravel(int i, Room dest) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    if (dest==Room::D1) {
        if (m_d1Closed) {
            LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+
                     " stopped before D1 because the door is already closed.");
            StartReturn(i);
            return false;
        }
        if (m_d1Owner!=-1 && m_d1Owner!=i) { a.destination=dest; ResolveTargetBlocked(i); return false; }
        m_d1Owner=i;
    }
    if (dest==Room::W2) {
        if (m_d2Closed) {
            LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+
                     " stopped before W2 because D2 is already closed.");
            StartReturn(i);
            return false;
        }
        if (m_w2Owner!=-1 && m_w2Owner!=i) { a.destination=dest; ResolveTargetBlocked(i); return false; }
        m_w2Owner=i;
    }
    a.destination=dest; a.travelling=true;
    a.timer=(a.character==Character::Pirate && a.room==Room::Backyard && dest==Room::Kitchen)
        ? 1.3f : RandomTravelTime(); // Skelly uses Pirate's travel range until his is specified.
    a.travelDuration=a.timer;
    LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+" moving: "+
             RoomName(a.room)+" -> "+RoomName(dest)+" ("+TextFormat("%.1fs",a.timer)+").");
    return true;
}

void RohansNightMode::Arrive(int i, Audio& audio) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    a.room=a.destination; a.history.push_back(a.room); a.travelling=false; a.defended=false;
    LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+" arrived in "+RoomName(a.room)+".");
    if (a.room==Room::D1) {
        // A door that is already shut must protect the office too. Keep the
        // encounter timer for a visible response window, but don't discard the
        // player's defense just because it was set before the final approach.
        a.defended=m_d1Closed;
        a.phase=Phase::EncounterD1; a.timer=ENCOUNTER_SECONDS;
        LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+
                 TextFormat(" started D1's 3-second encounter; D1 is %s.",m_d1Closed?"already closed":"open"));
        if (a.character==Character::Pirate) audio.PlayDoorSound(false);
        return;
    }
    if (a.room==Room::W2) {
        a.defended=m_d2Closed;
        a.phase=Phase::EncounterW2; a.timer=ENCOUNTER_SECONDS;
        LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+
                 TextFormat(" tapped W2; 3-second timer started; D2 is %s.",m_d2Closed?"already closed":"open"));
        audio.PlayWindowScare(); return;
    }
    a.phase=Phase::Roaming;
    a.timer=(a.room==Room::ParentsRoom && a.character==Character::Skelly) ? 4.0f : RandomRoomWait();
}

void RohansNightMode::StartReturn(int i) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+" defended; returning along recorded route.");
    if (m_d1Owner==i) m_d1Owner=-1;
    if (m_w2Owner==i) m_w2Owner=-1;
    a.phase=Phase::Returning; a.travelling=false; a.route.clear();
    a.returnIndex=static_cast<int>(a.history.size())-2; a.timer=RandomRoomWait();
}

void RohansNightMode::AdvanceAfterReturn(int i) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    if (a.returnIndex<0) {
        const Character c=a.character; a=Agent{}; a.character=c;
        a.room=c==Character::Pirate ? Room::Backyard : Room::LivingRoom;
        a.history={a.room}; a.timer=RandomRoomWait(); return;
    }
    a.room=a.history[static_cast<size_t>(a.returnIndex--)]; a.timer=RandomRoomWait();
    LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+" retraced to "+RoomName(a.room)+".");
    if (a.room==(a.character==Character::Pirate ? Room::Backyard : Room::LivingRoom)) a.returnIndex=-1;
}

void RohansNightMode::TriggerJumpscare(int i, Audio& audio, bool blackoutFirst) {
    std::string cause="power depletion";
    if (i>=0 && i<static_cast<int>(m_agents.size())) {
        const Agent& attacker=m_agents[static_cast<size_t>(i)];
        if (attacker.phase==Phase::EncounterD1) cause="missed D1 encounter";
        else if (attacker.phase==Phase::EncounterW2) cause="missed W2 encounter";
    }
    LogEvent(std::string(i==0?"Pirate Rohan":"Skelly Rohan")+" attack: "+cause+".");
    m_jumpscareCharacter=i; m_d1Owner=m_w2Owner=-1; m_monitorUp=false; m_d1Closed=m_d2Closed=false;
    if (blackoutFirst) { m_screen=Screen::Blackout; m_blackoutTimer=1.25f; audio.PlayPowerOut(); }
    else { m_screen=Screen::Jumpscare; m_jumpscareTimer=0.0f; audio.PlayJumpscare(); }
}

void RohansNightMode::UpdateAgent(int i, float dt, Audio& audio) {
    Agent& a=m_agents[static_cast<size_t>(i)];
    if (a.phase==Phase::EncounterD1) {
        a.timer-=dt;
        if (a.timer<=0.0f) {
            if (a.defended && m_d1Closed) {
                if (i==0) {
                    LogEvent("Pirate was blocked at D1; starting the specified extra 3-second wait.");
                    a.phase=Phase::D1ExtraWait; a.timer=3.0f;
                } else {
                    LogEvent("Skelly was blocked at D1; retreating along the recorded route.");
                    StartReturn(i);
                }
            } else {
                TriggerJumpscare(i,audio);
            }
        }
        return;
    }
    if (a.phase==Phase::D1ExtraWait) { a.timer-=dt; if (a.timer<=0.0f) StartReturn(i); return; }
    if (a.phase==Phase::EncounterW2) {
        a.timer-=dt;
        if (a.timer<=0.0f) {
            if (a.defended && m_d2Closed) {
                LogEvent(std::string(a.character==Character::Pirate?"Pirate":"Skelly")+
                         " was blocked at W2 by the closed D2 defense.");
                StartReturn(i);
            } else {
                TriggerJumpscare(i,audio,true);
            }
        }
        return;
    }
    if (a.phase==Phase::Returning) { a.timer-=dt; if (a.timer<=0.0f) AdvanceAfterReturn(i); return; }
    if (a.travelling && a.destination==Room::D1 && m_d1Closed) {
        LogEvent(std::string(i==0?"Pirate":"Skelly")+" stopped outside D1 when the door closed during approach.");
        StartReturn(i);
        return;
    }
    if (a.travelling && a.destination==Room::W2 && m_d2Closed) {
        LogEvent(std::string(i==0?"Pirate":"Skelly")+" stopped outside W2 when D2 closed during approach.");
        StartReturn(i);
        return;
    }
    a.timer-=dt;
    if (a.timer>0.0f) return;
    if (a.travelling) { Arrive(i,audio); return; }
    Room dest;
    if (!SelectNextRoom(i,dest)) { a.timer=RandomRoomWait(); return; }
    BeginTravel(i,dest);
}

void RohansNightMode::Update(float dt, Audio& audio) {
    m_staticTimer+=dt;
    if (m_screen==Screen::Intro) { m_introTimer+=dt; if (m_introTimer>=3.0f) m_screen=Screen::Playing; return; }
    if (m_screen==Screen::Blackout) {
        m_blackoutTimer-=dt;
        if (m_blackoutTimer<=0.0f) { m_screen=Screen::Jumpscare; m_jumpscareTimer=0.0f; audio.PlayJumpscare(); }
        return;
    }
    if (m_screen==Screen::Jumpscare) { m_jumpscareTimer+=dt; if (m_jumpscareTimer>=1.6f) m_screen=Screen::GameOver; return; }
    if (m_screen!=Screen::Playing) return;
    m_nightTimer+=dt;
    if (m_nightTimer>=NIGHT_SECONDS) { m_nightTimer=NIGHT_SECONDS; m_screen=Screen::NightComplete; LogEvent("Reached 6 AM; night survived."); audio.PlayChime6AM(); return; }
    const float drain=0.055f+(m_monitorUp?0.10f:0.0f)+(m_d1Closed?0.08f:0.0f)+(m_d2Closed?0.08f:0.0f);
    m_powerLogTimer+=dt;
    if (m_powerLogTimer>=5.0f) {
        m_powerLogTimer=0.0f;
        LogEvent(TextFormat("Power %.1f%%; drain %.3f%%/s (base .055, monitor +.10, D1 +.08, D2 +.08).",m_power,drain));
    }
    m_power=std::max(0.0f,m_power-drain*dt);
    if (m_power<=0.0f) { TriggerJumpscare(0,audio,true); return; }
    UpdateAgent(0,dt,audio);
    if (m_screen==Screen::Playing) UpdateAgent(1,dt,audio);
}

void RohansNightMode::DrawButton(Rectangle r, const char* label, bool active) const {
    DrawRectangleRounded(r,0.12f,8,active?Color{28,76,91,240}:Color{5,8,12,230});
    DrawRectangleRoundedLines(r,0.12f,8,active?kCyan:Color{170,185,196,210});
    const int size=20, width=MeasureText(label,size);
    DrawText(label,static_cast<int>(r.x+(r.width-width)*0.5f),static_cast<int>(r.y+(r.height-size)*0.5f),size,RAYWHITE);
}

void RohansNightMode::RenderThreatSilhouette(float x, float y, float s, Character c, bool bright) const {
    const Color body=c==Character::Pirate?Color{104,55,43,255}:Color{65,87,96,255};
    const Color eye=bright?Color{255,230,160,255}:Color{225,48,48,255};
    DrawEllipse(static_cast<int>(x),static_cast<int>(y+70*s),35*s,14*s,Color{0,0,0,130});
    DrawRectangleRounded(Rectangle{x-25*s,y+24*s,50*s,55*s},0.18f,5,body);
    DrawCircle(static_cast<int>(x),static_cast<int>(y+8*s),23*s,body);
    DrawRectangle(static_cast<int>(x-18*s),static_cast<int>(y+61*s),13*s,25*s,body);
    DrawRectangle(static_cast<int>(x+5*s),static_cast<int>(y+61*s),13*s,25*s,body);
    DrawCircle(static_cast<int>(x-8*s),static_cast<int>(y+7*s),3*s,eye);
    DrawCircle(static_cast<int>(x+8*s),static_cast<int>(y+7*s),3*s,eye);
    if (c==Character::Pirate) {
        DrawRectangle(static_cast<int>(x-20*s),static_cast<int>(y-20*s),40*s,6*s,body);
        DrawRectangle(static_cast<int>(x-8*s),static_cast<int>(y-31*s),18*s,12*s,body);
    }
}

void RohansNightMode::RenderOfficeCharacter(const Agent& agent, bool atWindow) const {
    const bool approaching = (agent.phase == Phase::EncounterD1 && !atWindow) ||
                             (agent.phase == Phase::EncounterW2 && atWindow);
    if (!approaching) return;

    const Texture2D sprite = agent.character == Character::Pirate ? m_pirateOfficeImage : m_skellyOfficeImage;
    const bool hasSprite = agent.character == Character::Pirate ? m_hasPirateOfficeImage : m_hasSkellyOfficeImage;
    const float progress = std::clamp(1.0f - agent.timer / ENCOUNTER_SECONDS, 0.0f, 1.0f);
    const float scale = 0.32f + progress * 1.38f;
    const float alpha = std::clamp(160.0f + progress * 95.0f, 0.0f, 255.0f);
    const float centerX = atWindow ? 1181.0f : 121.0f;
    const float baseY = atWindow ? 400.0f : 525.0f;
    if (hasSprite) {
        const float width = 145.0f * scale;
        const float height = 230.0f * scale;
        DrawTexturePro(sprite,
                       Rectangle{0, 0, static_cast<float>(sprite.width), static_cast<float>(sprite.height)},
                       Rectangle{centerX - width * 0.5f, baseY - height, width, height},
                       Vector2{0, 0}, 0.0f, Color{255, 255, 255, static_cast<unsigned char>(alpha)});
    } else {
        RenderThreatSilhouette(centerX, baseY - 90.0f * scale, scale, agent.character, false);
    }
}

void RohansNightMode::RenderOffice(bool crt) const {
    ClearBackground(Color{6,8,12,255});
    if (m_hasOfficeBackground) {
        DrawTexturePro(m_officeBackground,
                       Rectangle{0, 0, static_cast<float>(m_officeBackground.width), static_cast<float>(m_officeBackground.height)},
                       Rectangle{0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)},
                       Vector2{0, 0}, 0.0f, WHITE);
        if (m_d1Closed && m_d2Closed && m_hasBothDoorsClosedImage) {
            DrawTexturePro(m_bothDoorsClosedImage,
                           Rectangle{0, 0, static_cast<float>(m_bothDoorsClosedImage.width), static_cast<float>(m_bothDoorsClosedImage.height)},
                           Rectangle{0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)},
                           Vector2{0, 0}, 0.0f, WHITE);
        } else if (m_d1Closed && m_hasD1ClosedImage) {
            DrawTexturePro(m_d1ClosedImage,
                           Rectangle{0, 0, static_cast<float>(m_d1ClosedImage.width), static_cast<float>(m_d1ClosedImage.height)},
                           Rectangle{0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)},
                           Vector2{0, 0}, 0.0f, WHITE);
        } else if (m_d2Closed && m_hasD2ClosedImage) {
            DrawTexturePro(m_d2ClosedImage,
                           Rectangle{0, 0, static_cast<float>(m_d2ClosedImage.width), static_cast<float>(m_d2ClosedImage.height)},
                           Rectangle{0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)},
                           Vector2{0, 0}, 0.0f, WHITE);
        }
    } else {
    DrawRectangleGradientV(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Color{20,25,32,255},Color{4,6,9,255});
    DrawRectangle(0,0,SCREEN_WIDTH,185,Color{16,20,26,255});
    for(int x=40;x<SCREEN_WIDTH;x+=86) DrawLine(x,0,x-40,185,Color{35,39,45,255});
    DrawRectangle(0,185,SCREEN_WIDTH,12,Color{65,61,52,255});
    DrawRectangle(0,197,SCREEN_WIDTH,523,Color{10,12,16,255});
    // Left hall entrance (D1).
    DrawRectangle(28,240,185,298,Color{2,4,7,255});
    DrawRectangleLinesEx(Rectangle{28,240,185,298},5,Color{83,91,98,255});
    if(m_d1Closed) { DrawRectangle(34,247,173,284,Color{80,69,54,255}); DrawLine(42,265,198,515,Color{119,100,69,255}); DrawLine(198,265,42,515,Color{119,100,69,255}); }
    DrawText("D1  |  HALL 2",45,548,18,kPale);
    // Bathroom window W2 and adjacent D2 on the far wall.
    DrawRectangle(1040,245,205,280,Color{8,11,14,255});
    DrawRectangleLinesEx(Rectangle{1040,245,205,280},5,Color{80,91,100,255});
    DrawRectangle(1065,268,68,115,m_d2Closed?Color{71,63,53,255}:Color{23,46,54,255});
    DrawRectangleLinesEx(Rectangle{1065,268,68,115},3,m_d2Closed?Color{137,115,81,255}:kCyan);
    DrawRectangle(1145,268,72,115,Color{28,51,57,255});
    DrawRectangleLinesEx(Rectangle{1145,268,72,115},3,Color{74,152,165,255});
    DrawText("D2",1081,393,18,kPale); DrawText("W2",1160,393,18,kCyan);
    // Desk and CRT are deliberately code-drawn placeholders for future custom art.
    DrawRectangle(310,520,670,150,Color{31,27,24,255}); DrawRectangle(285,500,720,28,Color{79,62,48,255});
    DrawRectangleRounded(Rectangle{490,350,300,160},0.06f,5,Color{3,7,10,255});
    DrawRectangleLinesEx(Rectangle{490,350,300,160},8,Color{92,101,105,255});
    DrawRectangle(515,375,250,112,Color{4,12,16,255});
    DrawText("HOUSE CAMERA SYSTEM",538,414,20,Color{104,154,160,255});
    DrawText("SPACE: RAISE MONITOR",547,443,17,Color{135,148,148,255});
    DrawRectangle(615,510,48,23,Color{58,61,63,255}); DrawRectangle(565,532,148,15,Color{64,67,69,255});
    DrawCircle(326,472,30,Color{46,52,55,255}); DrawCircle(326,472,18,Color{15,17,18,255});
    }
    // Draw approaching characters behind the door artwork. W2 art should leave its
    // pane transparent so the sprite remains visible there while it approaches.
    // Threat sprites sit behind the translucent W2 pane and in front of the room background.
    for (const Agent& agent : m_agents) {
        RenderOfficeCharacter(agent, false);
        RenderOfficeCharacter(agent, true);
    }
    if (m_hasOfficeBackground) {
        const Texture2D* doorStateImage = nullptr;
        if (m_d1Closed && m_d2Closed && m_hasBothDoorsClosedImage) doorStateImage = &m_bothDoorsClosedImage;
        else if (m_d1Closed && m_hasD1ClosedImage) doorStateImage = &m_d1ClosedImage;
        else if (m_d2Closed && m_hasD2ClosedImage) doorStateImage = &m_d2ClosedImage;
        if (doorStateImage) {
            DrawTexturePro(*doorStateImage,
                           Rectangle{0, 0, static_cast<float>(doorStateImage->width), static_cast<float>(doorStateImage->height)},
                           Rectangle{0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)},
                           Vector2{0, 0}, 0.0f, WHITE);
        } else {
            if (m_d1Closed) {
                DrawRectangle(34,247,173,284,Color{80,69,54,255});
                DrawLine(42,265,198,515,Color{119,100,69,255});
                DrawLine(198,265,42,515,Color{119,100,69,255});
            }
            if (m_d2Closed) DrawRectangle(1065,268,68,115,Color{71,63,53,255});
        }
    }
    DrawText("ROHAN'S HOUSE  |  NIGHT 1",35,32,23,Color{180,189,194,255});
    const int hour=std::min(6,static_cast<int>(m_nightTimer/60.0f));
    DrawText(hour==0?"12 AM":TextFormat("%d AM",hour),1070,34,34,RAYWHITE);
    DrawText(TextFormat("POWER %02d%%",static_cast<int>(m_power)),1050,76,22,m_power<20?Color{255,90,70,255}:kPale);
    DrawRectangle(1050,105,180,9,Color{48,50,52,255});
    DrawRectangle(1050,105,static_cast<int>(180*m_power/100.0f),9,m_power<20?RED:Color{95,183,131,255});
    const float drain=0.055f+(m_monitorUp?0.10f:0.0f)+(m_d1Closed?0.08f:0.0f)+(m_d2Closed?0.08f:0.0f);
    DrawText(TextFormat("DRAIN %.3f%%/s",drain),1038,122,17,Color{155,174,177,255});
    for(const Agent& a:m_agents) {
        if(a.phase==Phase::EncounterD1) DrawText(TextFormat("D1 WARNING  %.1f",std::max(0.0f,a.timer)),230,248,20,Color{255,120,90,255});
        if(a.phase==Phase::EncounterW2) DrawText(TextFormat("W2 WARNING  %.1f",std::max(0.0f,a.timer)),855,248,20,Color{255,190,90,255});
    }
    DrawButton(kD1,m_d1Closed?"OPEN D1":"CLOSE D1",m_d1Closed);
    DrawButton(kMonitor,"RAISE MONITOR");
    DrawButton(kD2,m_d2Closed?"OPEN D2":"CLOSE D2 / W2",m_d2Closed);
    DrawText("Q: D1   SPACE: CAMERAS   E: D2/W2   ESC: PAUSE   F1: MAP   F3: LOG",315,700,17,Color{165,174,181,255});
    if(m_debugMapVisible||m_debugConsoleVisible)
        DrawText(TextFormat("F1 MAP %s  |  F3 CONSOLE %s  |  F2 ADVANCE PIRATE  |  F4 ADVANCE SKELLY",
                            m_debugMapVisible?"ON":"OFF",m_debugConsoleVisible?"ON":"OFF"),18,684,14,GREEN);
    if(crt) for(int y=0;y<SCREEN_HEIGHT;y+=4) DrawRectangle(0,y,SCREEN_WIDTH,1,Color{0,0,0,35});
}

void RohansNightMode::RenderRoomFeed(Rectangle r,int camera) const {
    const std::array<Color,5> walls{{Color{46,40,35,255},Color{39,48,42,255},Color{42,43,55,255},Color{39,43,47,255},Color{51,43,43,255}}};
    DrawRectangleRec(r,Color{5,8,10,255});
    Rectangle room{r.x+14,r.y+14,r.width-28,r.height-28};
    DrawRectangleRec(room,walls[static_cast<size_t>(camera)]);
    DrawRectangle(static_cast<int>(room.x),static_cast<int>(room.y+room.height*0.72f),static_cast<int>(room.width),static_cast<int>(room.height*0.28f),Color{25,25,25,255});
    DrawRectangle(static_cast<int>(room.x),static_cast<int>(room.y+room.height*0.2f),static_cast<int>(room.width),5,Color{78,73,63,255});
    if(camera==0) { DrawRectangle(static_cast<int>(room.x+80),static_cast<int>(room.y+35),130,65,Color{31,29,27,255}); DrawRectangle(static_cast<int>(room.x+room.width-170),static_cast<int>(room.y+55),90,110,Color{30,27,25,255}); DrawText("SOFA",static_cast<int>(room.x+105),static_cast<int>(room.y+115),17,kPale); }
    else if(camera==1) { for(int x=0;x<4;++x) DrawRectangle(static_cast<int>(room.x+45+x*125),static_cast<int>(room.y+120),74,52,Color{49,44,35,255}); DrawText("TABLES",static_cast<int>(room.x+35),static_cast<int>(room.y+45),17,kPale); }
    else if(camera==2) { DrawRectangle(static_cast<int>(room.x+120),static_cast<int>(room.y+35),240,120,Color{32,34,45,255}); DrawRectangle(static_cast<int>(room.x+room.width-120),static_cast<int>(room.y+45),58,150,Color{22,25,32,255}); }
    else if(camera==3) { for(int x=1;x<4;++x) DrawLine(static_cast<int>(room.x+x*room.width/4),static_cast<int>(room.y+18),static_cast<int>(room.x+x*room.width/4),static_cast<int>(room.y+room.height-20),Color{68,66,61,255}); DrawText("HALL 1",static_cast<int>(room.x+34),static_cast<int>(room.y+40),17,kPale); DrawText("HALL 2",static_cast<int>(room.x+room.width-120),static_cast<int>(room.y+40),17,kPale); }
    else { DrawRectangle(static_cast<int>(room.x+75),static_cast<int>(room.y+50),270,105,Color{44,37,34,255}); DrawRectangle(static_cast<int>(room.x+120),static_cast<int>(room.y+155),185,48,Color{27,24,22,255}); }
    bool shown=false;
    for(const Agent& a:m_agents) if(CameraShowsRoom(camera,a.room)) {
        RenderThreatSilhouette(room.x+room.width*(a.character==Character::Pirate?0.67f:0.38f),room.y+room.height*0.36f,1.22f,a.character,false); shown=true;
    }
    if(!shown && static_cast<int>(m_staticTimer*37.0f)%41==0)
        DrawRectangle(static_cast<int>(room.x+room.width*0.55f),static_cast<int>(room.y+room.height*0.3f),4,4,Color{160,165,165,100});
    DrawRectangleLinesEx(r,12,Color{18,22,25,255});
}

void RohansNightMode::RenderMap() const {
    Rectangle box{946,94,260,185}; DrawRectangleRec(box,Color{3,6,9,235}); DrawRectangleLinesEx(box,2,Color{105,121,130,230});
    DrawText("HOUSE MAP",964,105,16,kPale);
    const std::array<Rectangle,10> rooms{{{970,160,54,30},{1032,160,54,30},{1094,160,54,30},{970,198,54,30},{1032,198,54,30},{1094,198,54,30},{1156,160,38,30},{1156,198,38,30},{970,236,54,28},{1032,236,54,28}}};
    const std::array<const char*,10> names{{"C1","C2","C3","H1","C4","H3","H4","C5","H2","OFF"}};
    for(size_t i=0;i<rooms.size();++i) { DrawRectangleRec(rooms[i],Color{27,33,37,255}); DrawRectangleLinesEx(rooms[i],1,Color{99,113,119,255}); DrawText(names[i],static_cast<int>(rooms[i].x+5),static_cast<int>(rooms[i].y+8),13,kPale); }
    for(const Agent& a:m_agents) {
        int x=1059,y=250;
        switch(a.room) {
            case Room::LivingRoom:x=997;y=174;break; case Room::Kitchen:x=1059;y=174;break;
            case Room::AnjaliRoom:x=1121;y=174;break; case Room::Hallway1:x=997;y=212;break;
            case Room::Hallway2:x=1059;y=212;break; case Room::Hallway3:x=1121;y=212;break;
            case Room::Hallway4:x=1175;y=174;break; case Room::ParentsRoom:x=1175;y=212;break;
            case Room::Bathroom:x=997;y=250;break; default:break;
        }
        DrawCircle(x,y,6,a.character==Character::Pirate?kOrange:kCyan);
    }
}

void RohansNightMode::RenderDebugMap() const {
    const Color door{244,105,190,255}, window{73,166,245,255}, wall{89,103,111,255};
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Color{0,2,5,225});
    DrawRectangle(30,52,1220,610,Color{7,11,15,250});
    DrawRectangleLinesEx(Rectangle{30,52,1220,610},2,Color{115,132,141,255});
    DrawText("F1 | ROHAN'S HOUSE — LIVE MAP",55,66,25,RAYWHITE);
    DrawText("Pink doors | Blue windows | Dashed paths: orange Pirate, cyan Skelly | Moving dots show current positions",55,97,15,kPale);

    // Schematic room connections based on the supplied Game_Diagram and AI spec.
    DrawLineEx({180,175},{180,205},4,wall);       // Backyard / Kitchen, D4
    DrawLineEx({320,270},{355,320},4,wall);       // Kitchen / Hallway 1, D9
    DrawLineEx({310,500},{355,500},4,wall);       // Living Room / Hallway 1, D8
    DrawLineEx({402,440},{402,465},4,wall);       // H1 / H2 open passage
    DrawLineEx({650,465},{690,455},4,wall);       // H2 / H3 open passage
    DrawLineEx({695,350},{720,330},4,wall);       // H3 / H4 open passage
    DrawLineEx({735,270},{735,250},4,wall);       // H4 / Anjali, D6
    DrawLineEx({880,175},{895,175},4,wall);       // Anjali / Bathroom, D7
    DrawLineEx({610,545},{610,565},4,wall);       // H2 / Parents, D5
    DrawLineEx({860,500},{895,500},4,wall);       // H2 / Camera Room, D1
    DrawLineEx({1050,250},{1050,390},4,wall);    // Bathroom / Camera Room boundary

    auto room=[&](Rectangle r,const char* title,const char* camera) {
        DrawRectangleRec(r,Color{27,34,39,255});
        DrawRectangleLinesEx(r,2,Color{112,126,133,255});
        DrawText(title,static_cast<int>(r.x+8),static_cast<int>(r.y+8),17,RAYWHITE);
        if(camera) DrawText(camera,static_cast<int>(r.x+8),static_cast<int>(r.y+r.height-23),15,kCyan);
    };
    room({70,110,230,65},"BACKYARD (OUTSIDE)",nullptr);
    room({70,205,250,130},"KITCHEN","C2");
    room({70,440,240,130},"LIVING ROOM","C1");
    room({355,280,95,285},"HALLWAY 1","C4");
    room({450,465,410,80},"HALLWAY 2","C4");
    room({650,350,90,105},"HALL 3",nullptr);
    room({590,270,290,60},"HALLWAY 4",nullptr);
    room({590,100,290,150},"ANJALI'S ROOM","C3");
    room({895,100,230,150},"BATHROOM",nullptr);
    room({505,565,230,70},"PARENTS ROOM  |  C5",nullptr);
    room({895,390,285,175},"CAMERA ROOM  |  PLAYER",nullptr);

    auto route=[&](const std::vector<Room>& path,Color color) {
        for(size_t i=1;i<path.size();++i) {
            const Vector2 a=MapPoint(path[i-1]), b=MapPoint(path[i]);
            const float dx=b.x-a.x,dy=b.y-a.y;
            const float length=std::sqrt(dx*dx+dy*dy);
            for(float d=0.0f;d<length;d+=22.0f) {
                const float end=std::min(d+12.0f,length);
                DrawLineEx({a.x+dx*d/length,a.y+dy*d/length},
                           {a.x+dx*end/length,a.y+dy*end/length},3,color);
            }
        }
    };
    route({Room::Backyard,Room::Kitchen,Room::Hallway1,Room::Hallway2,Room::D1},kOrange);
    route({Room::Hallway2,Room::Hallway3,Room::Hallway4,Room::AnjaliRoom,Room::Bathroom,Room::W2},kOrange);
    route({Room::LivingRoom,Room::Hallway1,Room::Hallway2,Room::ParentsRoom},kCyan);
    route({Room::Hallway2,Room::D1},kCyan);
    route({Room::Hallway2,Room::Hallway3,Room::Hallway4,Room::AnjaliRoom,Room::Bathroom,Room::W2},kCyan);

    auto marker=[&](float x,float y,const char* text,Color color) {
        DrawRectangle(static_cast<int>(x),static_cast<int>(y),38,23,color);
        DrawRectangleLinesEx(Rectangle{x,y,38,23},1,RAYWHITE);
        DrawText(text,static_cast<int>(x+3),static_cast<int>(y+4),14,Color{10,13,17,255});
    };
    marker(165,183,"D4",door); marker(330,300,"D9",door); marker(312,472,"D8",door);
    marker(595,546,"D5",door); marker(715,250,"D6",door); marker(858,160,"D7",door);
    marker(862,478,"D1",door); marker(1000,365,"D2",door); marker(1050,365,"W2",window);
    marker(1140,455,"W1",window);
    DrawText("D3 is absent in the supplied map.",925,580,15,Color{182,190,195,255});

    const std::array<Vector2,5> cams{{{180,510},{195,275},{755,220},{400,410},{620,600}}};
    for(int i=0;i<5;++i) {
        DrawCircle(static_cast<int>(cams[static_cast<size_t>(i)].x),static_cast<int>(cams[static_cast<size_t>(i)].y),13,kCyan);
        DrawText(TextFormat("C%d",i+1),static_cast<int>(cams[static_cast<size_t>(i)].x-9),static_cast<int>(cams[static_cast<size_t>(i)].y-7),14,Color{5,20,24,255});
    }

    for(size_t i=0;i<m_agents.size();++i) {
        const Agent& a=m_agents[i];
        Vector2 p=MapPoint(a.room);
        if(a.travelling && a.travelDuration>0.0f) {
            const Vector2 q=MapPoint(a.destination);
            const float t=1.0f-std::clamp(a.timer/a.travelDuration,0.0f,1.0f);
            DrawLineEx(p,q,3,i==0?kOrange:kCyan);
            p={p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t};
        }
        if(i==1) p.x+=19.0f;
        const Color dot=i==0?kOrange:kCyan;
        DrawCircle(static_cast<int>(p.x),static_cast<int>(p.y),14,Color{2,4,7,255});
        DrawCircle(static_cast<int>(p.x),static_cast<int>(p.y),10,dot);
        DrawCircleLines(static_cast<int>(p.x),static_cast<int>(p.y),12,RAYWHITE);
        DrawText(i==0?"P":"S",static_cast<int>(p.x-4),static_cast<int>(p.y-7),13,Color{8,12,15,255});
    }
    DrawText("F1 closes map  |  F3 opens event console  |  simulation continues while this overlay is visible",55,635,16,kPale);
}

void RohansNightMode::RenderDebugConsole() const {
    const Rectangle panel{650,48,600,614};
    DrawRectangleRec(panel,Color{3,7,11,248});
    DrawRectangleLinesEx(panel,2,Color{101,145,155,255});
    DrawText("F3 | LIVE GAMEPLAY CONSOLE",672,62,23,RAYWHITE);
    const float drain=0.055f+(m_monitorUp?0.10f:0.0f)+(m_d1Closed?0.08f:0.0f)+(m_d2Closed?0.08f:0.0f);
    DrawText(TextFormat("Time %02d:%02d   Power %.1f%%   Drain %.3f%%/s",
                        static_cast<int>(m_nightTimer/60.0f),static_cast<int>(m_nightTimer)%60,m_power,drain),
             672,96,17,Color{171,220,206,255});
    DrawText(TextFormat("Load = base .055 + monitor %.2f + D1 %.2f + D2 %.2f %%/sec",
                        m_monitorUp?0.10f:0.0f,m_d1Closed?0.08f:0.0f,m_d2Closed?0.08f:0.0f),
             672,119,16,kPale);
    DrawText(TextFormat("Doors: D1 %s (+.08 closed) | D2 %s (+.08 closed; W2 defense)",
                        m_d1Closed?"CLOSED":"OPEN",m_d2Closed?"CLOSED":"OPEN"),
             672,142,15,kPale);
    DrawText(TextFormat("Monitor: %s | Camera: C%d | D1 owner: %s | W2 owner: %s",
                        m_monitorUp?"UP":"DOWN",m_currentCamera+1,
                        m_d1Owner<0?"none":(m_d1Owner==0?"Pirate":"Skelly"),
                        m_w2Owner<0?"none":(m_w2Owner==0?"Pirate":"Skelly")),
             672,164,15,kPale);
    DrawText("Assumptions: equal route odds; Skelly uses Pirate's 1.3–5.0s travel range.",672,184,14,Color{148,170,178,255});

    for(int i=0;i<2;++i) {
        const Agent& a=m_agents[static_cast<size_t>(i)];
        const int y=205+i*86;
        const char* name=i==0?"PIRATE ROHAN":"SKELLY ROHAN";
        DrawText(name,672,y,17,i==0?kOrange:kCyan);
        std::string route;
        for(size_t j=a.routeIndex;j<a.route.size();++j) {
            if(!route.empty()) route += " > ";
            route += RoomName(a.route[j]);
        }
        if(route.empty()) route="(no queued room)";
        const char* next=a.travelling?RoomName(a.destination):
            (a.phase==Phase::Returning && a.returnIndex>=0?RoomName(a.history[static_cast<size_t>(a.returnIndex)]):"decision pending");
        DrawText(TextFormat("Room: %s  |  Phase: %s",RoomName(a.room),PhaseName(a.phase)),672,y+22,15,kPale);
        DrawText(TextFormat("Next: %s  |  %s timer: %.2fs  |  defended: %s",next,
                            a.travelling?"travel":"state",std::max(0.0f,a.timer),a.defended?"yes":"no"),
                 672,y+43,14,kPale);
        DrawText(TextFormat("Queued route: %s",route.c_str()),672,y+63,14,Color{169,184,191,255});
    }

    DrawText("RECENT EVENTS (newest at bottom)",672,382,16,Color{242,201,126,255});
    const size_t first=m_consoleLog.size()>9?m_consoleLog.size()-9:0;
    int y=408;
    for(size_t i=first;i<m_consoleLog.size();++i,y+=25)
        DrawText(m_consoleLog[i].c_str(),672,y,14,Color{211,220,225,255});
    DrawText("F1 map  |  F3 close console  |  F2 advance Pirate  |  F4 advance Skelly",672,638,14,Color{135,165,173,255});
}

void RohansNightMode::RenderMonitor(bool crt) const {
    DrawRectangle(42,30,1196,654,Color{2,4,7,245}); DrawRectangleLinesEx(Rectangle{42,30,1196,654},8,Color{68,77,82,255});
    RenderRoomFeed(Rectangle{78,68,1124,510},m_currentCamera);
    static const char* labels[]={"C1 | LIVING ROOM","C2 | KITCHEN","C3 | ANJALI'S ROOM","C4 | HALLWAYS 1 + 2","C5 | PARENTS ROOM"};
    DrawText(labels[m_currentCamera],92,78,21,Color{209,220,222,255});
    const float drain=0.055f+0.10f+(m_d1Closed?0.08f:0.0f)+(m_d2Closed?0.08f:0.0f);
    DrawText(TextFormat("POWER %02d%%   DRAIN %.3f%%/s",static_cast<int>(m_power),drain),940,39,18,
             m_power<20.0f?Color{255,112,94,255}:kPale);
    DrawText("BACKYARD • HALLS 3/4 • BATHROOM • D1/W2 NOT ON THESE FEEDS",102,540,17,Color{179,183,174,255});
    for(int i=0;i<5;++i) DrawButton(kCamButtons[static_cast<size_t>(i)],TextFormat("C%d  %s",i+1,i==m_currentCamera?"VIEWING":"SELECT"),i==m_currentCamera);
    DrawButton(kMonitor,"LOWER MONITOR  [SPACE]",true);
    if(crt) for(int y=0;y<650;y+=5) DrawRectangle(50,y,1180,1,Color{0,0,0,45});
    RenderMap();
}

void RohansNightMode::RenderPause() const {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Color{0,0,0,175}); DrawText("PAUSED",510,255,56,RAYWHITE);
    DrawButton(kResume,"RESUME",true); DrawButton(kPauseMenu,"MAIN MENU");
}

void RohansNightMode::Render(bool crt) const {
    if(m_screen==Screen::Inactive) { ClearBackground(BLACK); return; }
    if(m_screen==Screen::Intro) {
        ClearBackground(Color{2,4,7,255}); DrawRectangle(72,78,1136,560,Color{8,11,14,255}); DrawRectangleLinesEx(Rectangle{72,78,1136,560},2,Color{83,94,99,255});
        DrawText("ROHAN'S NIGHT",397,225,54,RAYWHITE); DrawText("A separate house-game prototype",434,305,26,Color{166,181,186,255});
        DrawText("PIRATE ROHAN  •  SKELLY ROHAN  •  FIVE HOUSE CAMERAS",267,400,22,Color{121,173,180,255});
        DrawText("Q CLOSES D1   |   E CLOSES D2 FOR W2   |   SPACE RAISES MONITOR",227,482,20,kPale);
        DrawText("ESC PAUSES",548,550,20,Color{155,166,172,255}); return;
    }
    if(m_screen==Screen::Playing || m_screen==Screen::Paused) {
        if(m_monitorUp) RenderMonitor(crt); else RenderOffice(crt);
        DrawRectangleRounded(kPause,0.16f,6,Color{4,7,10,220}); DrawRectangleRoundedLines(kPause,0.16f,6,Color{186,197,202,210});
        DrawRectangle(1207,41,7,25,RAYWHITE); DrawRectangle(1223,41,7,25,RAYWHITE);
        if(m_screen==Screen::Paused) RenderPause();
        if(m_debugMapVisible) RenderDebugMap();
        if(m_debugConsoleVisible) RenderDebugConsole();
        return;
    }
    if(m_screen==Screen::Blackout) { ClearBackground(BLACK); DrawText("POWER OUT",525,335,38,Color{105,109,109,255}); return; }
    if(m_screen==Screen::Jumpscare) {
        ClearBackground((static_cast<int>(m_jumpscareTimer*18.0f)%2==0)?Color{34,4,8,255}:BLACK);
        RenderThreatSilhouette(640,170,3.4f+std::sin(m_jumpscareTimer*36.0f)*0.25f,m_jumpscareCharacter==0?Character::Pirate:Character::Skelly,true);
        DrawText("!",605,490,84,RAYWHITE); return;
    }
    ClearBackground(Color{3,5,8,255}); const bool win=m_screen==Screen::NightComplete;
    DrawText(win?"6 AM":"GAME OVER",win?570:485,235,62,RAYWHITE);
    DrawText(win?"You survived the night.":"The house went quiet.",455,330,26,Color{177,187,193,255});
    DrawButton(kRetry,win?"PLAY AGAIN":"RETRY NIGHT"); DrawButton(kMenu,"MAIN MENU");
}

bool RohansNightMode::ConsumeReturnToMenu() { const bool result=m_returnToMenu; m_returnToMenu=false; return result; }

} // namespace FNAF
