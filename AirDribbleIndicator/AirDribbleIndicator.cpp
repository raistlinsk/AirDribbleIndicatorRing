#include "pch.h"
#include "AirDribbleIndicator.h"
#include <cmath>
#include <deque>
#include <chrono>

BAKKESMOD_PLUGIN(AirDribbleIndicator, "Air Dribble Indicator Ring", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

constexpr float PI = 3.14159265358979323846f;
constexpr float BALL_RADIUS = 92.75f;

// =====================================================
// Touch marker structure
// =====================================================
struct TouchMarker {
    Vector offset;   // Ball-relative offset
    std::chrono::steady_clock::time_point created;
    bool isFlipReset;
};

static std::deque<TouchMarker> touchMarkers;

std::chrono::steady_clock::time_point lastFlipResetTime;
std::chrono::steady_clock::time_point lastBallHitTime;

bool inValidGame = false;
bool lastHasFlip = true;

// =====================================================
// OnHitBall native param struct
// =====================================================
struct OnHitBallParams {
    uintptr_t Ball;
    Vector HitLocation;
    Vector HitNormal;
};


// =====================================================
// -------- Forward declarations (helpers) --------------
// =====================================================
void DrawRing(CanvasWrapper& canvas, BallWrapper& ball);
void DrawTouchMarkers(CanvasWrapper& canvas, BallWrapper& ball);
void AddTouchMarker(const Vector& contactPoint, BallWrapper& ball, bool isFlipReset);

Vector ComputeFlipResetContactPoint(CarWrapper& car, BallWrapper& ball);

bool IsValidGame(std::shared_ptr<GameWrapper> gw);


// =====================================================
// ---------------- Plugin lifecycle --------------------
// =====================================================
void AirDribbleIndicator::onLoad()
{
    _globalCvarManager = cvarManager;

    // ---------------- Ring CVars ----------------
    cvarManager->registerCvar("ring_enabled", "1", "Enable ring");
    cvarManager->registerCvar("ring_size", "15", "Ring size", true, true, 10, true, 80);
    cvarManager->registerCvar("ring_color", "#999999", "Ring color");

    // ---------------- Touch marker CVars ----------------
    cvarManager->registerCvar("touch_marker_enabled", "1", "Enable touch markers");
    cvarManager->registerCvar("touch_marker_correct_color", "#008000", "Correct touch");
    cvarManager->registerCvar("touch_marker_incorrect_color", "#800000", "Incorrect touch");
    cvarManager->registerCvar("touch_marker_reset_color", "#F8F833", "Flip reset touch");
    cvarManager->registerCvar("touch_marker_duration", "1.0", "Marker duration (s)", true, true, 0.5, true, 5);
    cvarManager->registerCvar("max_touch_markers", "5", "Max active touch markers", true, true, 1, true, 10);
    cvarManager->registerCvar("touch_marker_size", "8.0", "Marker size (px)", true, true, 2, true, 15);

    // Debug notifier
    cvarManager->registerNotifier("Initialize",
        [this](std::vector<std::string>) { Initialize(); },
        "", PERMISSION_ALL);

    // Rendering
    gameWrapper->RegisterDrawable(
        std::bind(&AirDribbleIndicator::Render, this, std::placeholders::_1));

    // Events
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.StartRound",
        std::bind(&AirDribbleIndicator::OnGameStart, this, std::placeholders::_1));

    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed",
        std::bind(&AirDribbleIndicator::OnGameEnd, this, std::placeholders::_1));

    gameWrapper->HookEventWithCallerPost<CarWrapper>(
        "Function TAGame.Car_TA.OnHitBall",
        std::bind(&AirDribbleIndicator::OnHitBallHook, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void AirDribbleIndicator::Initialize()
{
    inValidGame = IsValidGame(gameWrapper);
}

void AirDribbleIndicator::OnGameStart(std::string)
{
    inValidGame = IsValidGame(gameWrapper);
}

void AirDribbleIndicator::OnGameEnd(std::string)
{
    inValidGame = false;
}


// =====================================================
// ------------------ Main Render ----------------------
// =====================================================
void AirDribbleIndicator::Render(CanvasWrapper canvas)
{
    if (!IsValidGame(gameWrapper)) return;

    ServerWrapper server = gameWrapper->GetGameEventAsServer();
    if (!server) return;

    BallWrapper ball = server.GetBall();
    if (!ball) return;

    // --- Flip reset detection ---
    CheckFlipReset(gameWrapper);

    // --- Ring ---
    if (cvarManager->getCvar("ring_enabled").getBoolValue())
        DrawRing(canvas, ball);

    // --- Touch markers ---
    if (cvarManager->getCvar("touch_marker_enabled").getBoolValue())
        DrawTouchMarkers(canvas, ball);
}


// =====================================================
// --------------- Flip Reset Detection ----------------
// =====================================================
void AirDribbleIndicator::CheckFlipReset(std::shared_ptr<GameWrapper> gw)
{
    if (!IsValidGame(gw)) return;

    CarWrapper car = gw->GetLocalCar();
    if (!car) return;

    ServerWrapper server = gw->GetGameEventAsServer();
    if (!server) return;

    BallWrapper ball = server.GetBall();
    if (!ball) return;

    bool hasFlip = car.HasFlip() != 0;
    bool isGround = car.AnyWheelTouchingGround();

    bool isFlipResetMoment = (!lastHasFlip && hasFlip && !isGround);
    lastHasFlip = hasFlip;

    if (!isFlipResetMoment) return;

    
    Vector carPos = car.GetLocation();
    Vector ballPos = ball.GetLocation();

    // --- Reject impossible distances (fake flip resets from reset) ---
    float distCarBall = (carPos - ballPos).magnitude();
    if (distCarBall > 250.0) return; // Car is too far from ball -> this is NOT a real flip reset

    // Compute contact point on the BALL, not the car
    Vector contactPoint = ComputeFlipResetContactPoint(car, ball);
    AddTouchMarker(contactPoint, ball, true);
}


// =====================================================
// ---------------- OnHitBall Hook ----------------------
// =====================================================
void AirDribbleIndicator::OnHitBallHook(CarWrapper caller, void* params, std::string)
{
    if (!IsValidGame(gameWrapper)) return;
    if (!cvarManager->getCvar("touch_marker_enabled").getBoolValue()) return;
    if (!params) return;

    OnHitBallParams* p = reinterpret_cast<OnHitBallParams*>(params);
    BallWrapper ball = BallWrapper(p->Ball);
    if (!ball) return;

    auto now = std::chrono::steady_clock::now();
    lastBallHitTime = now;

    bool wasFlipReset = std::abs(std::chrono::duration<float>(now - lastFlipResetTime).count()) < 0.08f;

    AddTouchMarker(p->HitLocation, ball, wasFlipReset);
}


// =====================================================
// ------------------ Drawing: Ring ---------------------
// =====================================================
void DrawRing(CanvasWrapper& canvas, BallWrapper& ball)
{
    float size = _globalCvarManager->getCvar("ring_size").getFloatValue();
    LinearColor color = _globalCvarManager->getCvar("ring_color").getColorValue();

    Vector ballLoc = ball.GetLocation();

    float slice = BALL_RADIUS - size;
    float ringRad = sqrtf(BALL_RADIUS * BALL_RADIUS - slice * slice) + 2.0f;

    Vector center3D(ballLoc.X, ballLoc.Y, ballLoc.Z - BALL_RADIUS + size);

    canvas.SetColor(color);

    const int segments = 42;
    const float width = 8.0f;
    const float step = 2.f * PI / segments;
    const float overlap = 0.10f;

    for (int i = 0; i < segments; i++)
    {
        float a0 = i * step - overlap * step * 0.5f;
        float a1 = (i + 1) * step + overlap * step * 0.5f;

        Vector p0(
            center3D.X + ringRad * cosf(a0),
            center3D.Y + ringRad * sinf(a0),
            center3D.Z);

        Vector p1(
            center3D.X + ringRad * cosf(a1),
            center3D.Y + ringRad * sinf(a1),
            center3D.Z);

        canvas.DrawLine(canvas.ProjectF(p0), canvas.ProjectF(p1), width);
    }
}


// =====================================================
// ------------- Drawing: Touch Markers -----------------
// =====================================================
void DrawTouchMarkers(CanvasWrapper& canvas, BallWrapper& ball)
{
    float duration = _globalCvarManager->getCvar("touch_marker_duration").getFloatValue();
    float sizePx = _globalCvarManager->getCvar("touch_marker_size").getFloatValue();

    LinearColor correctColor = _globalCvarManager->getCvar("touch_marker_correct_color").getColorValue();
    LinearColor incorrectColor = _globalCvarManager->getCvar("touch_marker_incorrect_color").getColorValue();
    LinearColor resetColor = _globalCvarManager->getCvar("touch_marker_reset_color").getColorValue();

    auto  now = std::chrono::steady_clock::now();
    Vector ballCenter = ball.GetLocation();

    // Remove expired markers
    while (!touchMarkers.empty() &&
        std::chrono::duration<float>(now - touchMarkers.front().created).count() > duration)
    {
        touchMarkers.pop_front();
    }

    float ringSize = _globalCvarManager->getCvar("ring_size").getFloatValue();
    float ringZ = ballCenter.Z - BALL_RADIUS + ringSize;

    for (const auto& m : touchMarkers)
    {
        Vector worldPos = ballCenter + m.offset;
        Vector2F p2 = canvas.ProjectF(worldPos);

        Vector2F L = { p2.X - sizePx, p2.Y };
        Vector2F R = { p2.X + sizePx, p2.Y };
        Vector2F U = { p2.X,         p2.Y - sizePx };
        Vector2F D = { p2.X,         p2.Y + sizePx };

        if (m.isFlipReset)
            canvas.SetColor(resetColor);
        else if (worldPos.Z > ringZ)
            canvas.SetColor(incorrectColor);
        else
            canvas.SetColor(correctColor);

        float thickness = 4.0f;
        canvas.DrawLine(L, R, thickness);
        canvas.DrawLine(U, D, thickness);
    }
}


// =====================================================
// -------------- Add a touch marker --------------------
// =====================================================
void AddTouchMarker(const Vector& contactPoint, BallWrapper& ball, bool isFlipReset)
{
    TouchMarker m;
    m.offset = contactPoint - ball.GetLocation();
    m.created = std::chrono::steady_clock::now();
    m.isFlipReset = isFlipReset;

    touchMarkers.push_back(m);

    int maxMarkers = _globalCvarManager->getCvar("max_touch_markers").getIntValue();
    while ((int)touchMarkers.size() > maxMarkers)
        touchMarkers.pop_front();
}


// =====================================================
// -------------- Flip reset ball contact ---------------
// =====================================================
Vector ComputeFlipResetContactPoint(CarWrapper& car, BallWrapper& ball)
{
    Vector carPos = car.GetLocation();
    Vector ballCenter = ball.GetLocation();

    Vector dir = carPos - ballCenter;
    float len = dir.magnitude();

    if (len < 1e-3f)               // Avoid division by zero
        return ballCenter;

    Vector dirNorm = dir / len;
    return ballCenter + dirNorm * BALL_RADIUS;
}


// =====================================================
// ---------------- Valid game check -------------------
// =====================================================
bool IsValidGame(std::shared_ptr<GameWrapper> gw)
{
    return gw && gw->IsInGame() && !gw->IsInOnlineGame();
}

