# Веха 1: движение и стрельба по сети — план реализации

> **For agentic workers:** REQUIRED SUB-SKILL: Use executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Два игрока по сети бегают серыми капсулами со всеми видами движения (стамина, рывок, двойной прыжок, скольжение, зиплайн), стреляют из одного hitscan-оружия с компенсацией лага, а сервер скрывает врагов, которых команда не видит.

**Architecture:** Вся игровая математика лежит в модуле `BrutusSim`: чистый C++17 без UObject и без зависимостей от движка. Он собирается и в UE (как модуль), и здесь, в контейнере (CMake + doctest). Поэтому он покрыт юнит-тестами, которые Claude запускает сам. Модуль `Brutus` — тонкая UE-обвязка: CharacterMovementComponent с предсказанием, GAS, подсистемы компенсации лага и видимости. Её собирает и проверяет автор в редакторе по чек-листам, потому что Unreal Editor в контейнере не запускается.

**Tech Stack:** Unreal Engine 5 (последний стабильный 5.x), C++17/20, Gameplay Ability System, Enhanced Input; doctest 2.4.11 и CMake для нативных тестов `BrutusSim`.

**Spec:** `docs/design.md`, разделы 3, 5, 7, 9 (веха 1).

## Global Constraints

- Единицы: сантиметры, секунды, ось Z вверх (как в UE).
- `BrutusSim` не включает ни одного заголовка UE. Только стандартная библиотека и `BrutusSimApi.h`. Без исключений и RTTI (UE собирает без них).
- Нативная сборка тестов: `-Wall -Wextra -Wshadow -Werror`. Это приближение к строгости UBT.
- Сервер решает всё: урон, видимость, расход стамины. Клиент только предсказывает.
- Код совместим и с listen server, и с dedicated server. Цель `BrutusServer` собирается только движком из исходников; с движком из лаунчера проверяем listen server.
- История хитбоксов хранится 3 с (`HistorySeconds = 3.0`). Откат выстрела ограничен 0.25 с (`MaxShotRewind = 0.25`).
- Ассеты `.uasset` не создаём из кода: геометрия, ввод и меши создаются в C++ во время выполнения или берутся из `/Engine/BasicShapes`. Единственный ручной ассет — пустая карта `/Game/Maps/M_Greybox`.
- Все числа баланса ниже — стартовые значения для настройки, а не требования спеки.

## Review Focus

1. **Высокий пинг (150 мс) и стрельба по бегущей цели.** Попадание засчитывается там, где стрелок видел цель. Тест: `HitboxHistory` интерполирует между снимками (Task 5), откат клампится до 0.25 с (Task 6); ручной чек в Task 12 с `NetEmulation.PktLag 150`.
2. **Спам рывка при нулевой стамине на клиенте с потерей пакетов.** Сервер отклоняет рывок, клиент корректируется без бесконечного дёрганья. Тест: `Stamina` не уходит в минус (Task 2); ручной чек в Task 9 с `PktLoss 5`.
3. **Выстрел в цель, которая только что умерла и возродилась.** Пуля не попадает в возрождённого героя на старом месте. Тест: разрыв (teleport) в `HitboxHistory` не интерполируется (Task 5).
4. **Подделка времени выстрела клиентом** (время из будущего или далёкого прошлого). Сервер клампит. Тест: `ClampRewindTime` (Task 6).
5. **Враг выходит из-за угла.** Он не появляется с задержкой больше ~0.1 с после выхода в прямую видимость и не исчезает мгновенно, когда скрылся. Тест: grace-период в `TeamVision` (Task 7); ручной чек в Task 13.

---

## Структура файлов

```
BRUTUS/
  Brutus.uproject
  Config/DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini, DefaultGameplayTags.ini
  Source/
    Brutus.Target.cs, BrutusEditor.Target.cs, BrutusServer.Target.cs
    BrutusSim/                      — чистая математика, без UE
      BrutusSim.Build.cs
      Public/BrutusSimApi.h, Vec3.h, Stamina.h, MoveRules.h, Zipline.h,
             HitboxHistory.h, Raycast.h, TeamVision.h
      Private/BrutusSimModule.cpp, Stamina.cpp, MoveRules.cpp, Zipline.cpp,
              HitboxHistory.cpp, Raycast.cpp, TeamVision.cpp
    Brutus/                         — UE-обвязка
      Brutus.Build.cs, Brutus.h, Brutus.cpp
      Core/      BrutusGameMode, BrutusGameState, BrutusPlayerState, BrutusPlayerController, BrutusHUD, BrutusSimConvert.h
      Character/ BrutusCharacter, BrutusMovementComponent, BrutusHitboxComponent
      Movement/  BrutusZiplineComponent, BrutusZipline, BrutusZiplineRegistry
      Abilities/ BrutusAttributeSet, BrutusDamageEffect, BrutusGameplayTags
      Combat/    BrutusLagCompensationSubsystem, BrutusHitscanWeaponComponent
      Vision/    BrutusTeamVisionSubsystem
      Arena/     BrutusGreyboxArena
  SimTests/                         — нативные тесты BrutusSim (вне Source, UBT их не видит)
    CMakeLists.txt, third_party/doctest.h, test_*.cpp
```

---

### Task 1: Каркас репозитория и нативные тесты BrutusSim

**Files:**
- Create: `.gitignore` (UE: `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/ *.sln`; плюс `SimTests/build/`; плюс `RobloxAssets/`)
- Create: `Source/BrutusSim/Public/BrutusSimApi.h`, `Source/BrutusSim/Public/Vec3.h`
- Create: `SimTests/CMakeLists.txt`, `SimTests/third_party/doctest.h` (v2.4.11), `SimTests/test_vec3.cpp`, `SimTests/test_main.cpp`

**Interfaces:**
- Produces: `namespace bsim`, `struct Vec3 { float X, Y, Z; }` с операторами `+ - *(float) /(float)` и функциями `Dot(a,b)`, `Cross(a,b)`, `Length(v)`, `LengthSq(v)`, `Normalized(v)` (нулевой вектор → нулевой), `Lerp(a,b,t)`, `Flatten(v)` (Z = 0). `BRUTUSSIM_API` пустой, если не определён.
- CMake-цель `brutus_sim_tests` компилирует `Source/BrutusSim/Private/*.cpp` (кроме `BrutusSimModule.cpp`) и `SimTests/test_*.cpp`.

- [ ] **Step 1: Тест** `test_vec3.cpp`: `Length({3,4,0}) == 5`; `Normalized({0,0,0}) == {0,0,0}`; `Lerp({0,0,0},{10,0,0},0.25f).X == 2.5f`; `Flatten({1,2,3}).Z == 0`; `Dot({1,0,0},{0,1,0}) == 0`.
- [ ] **Step 2:** `cmake -S SimTests -B SimTests/build && cmake --build SimTests/build` → ошибка компиляции (`Vec3.h` нет).
- [ ] **Step 3:** Реализовать `Vec3.h` (header-only, `inline`).
- [ ] **Step 4:** `cmake --build SimTests/build && ./SimTests/build/brutus_sim_tests` → все тесты PASS.
- [ ] **Step 5:** Commit `chore: repo skeleton and native test harness for BrutusSim`.

### Task 2: Стамина

**Files:** Create `Public/Stamina.h`, `Private/Stamina.cpp`, `SimTests/test_stamina.cpp`

**Interfaces:**
- Produces:
  - `struct StaminaConfig { int MaxCharges = 3; float RegenSecondsPerCharge = 2.5f; };`
  - `struct StaminaState { float Charges = 3.f; };`
  - `bool TryConsume(StaminaState&, const StaminaConfig&, int Cost = 1);` — тратит, только если `Charges >= Cost`.
  - `void Regen(StaminaState&, const StaminaConfig&, float DeltaSeconds);` — клампит к `MaxCharges`.

- [ ] **Step 1: Тесты:** полная стамина позволяет 3 `TryConsume`, четвёртый возвращает `false` и `Charges == 0` (не отрицательная); после `Regen(2.5)` — `Charges == 1`; `Regen(1.25)` от 0 даёт 0.5 и `TryConsume` → `false`; `Regen(100)` → ровно 3; `TryConsume(state, cfg, 2)` при 1.9 → `false`, заряды не меняются.
- [ ] **Step 2:** Сборка и запуск → FAIL (нет символов).
- [ ] **Step 3:** Реализовать.
- [ ] **Step 4:** Запуск → PASS.
- [ ] **Step 5:** Commit `feat(sim): stamina charges with regen`.

### Task 3: Правила движения: рывок, прыжок в воздухе, скольжение

**Files:** Create `Public/MoveRules.h`, `Private/MoveRules.cpp`, `SimTests/test_moverules.cpp`

**Interfaces:**
- Produces:
  - `struct MoveConfig { float DashSpeed = 1500.f; int AirJumps = 1; float SlideMinStartSpeed = 500.f; float SlideStartBoost = 250.f; float SlideDecel = 400.f; float SlideMaxSpeed = 1800.f; float SlideExitSpeed = 300.f; float Gravity = 980.f; };`
  - `Vec3 ComputeDashVelocity(const Vec3& WishDir, const Vec3& Facing, const MoveConfig&);` — горизонтальное направление `WishDir` (или `Facing`, если `WishDir` ≈ 0) × `DashSpeed`, `Z = 0`.
  - `struct AirJumpState { int Used = 0; };` `bool TryAirJump(AirJumpState&, const MoveConfig&);` `void ResetAirJumps(AirJumpState&);`
  - `bool CanStartSlide(float HorizontalSpeed, bool bGrounded, const MoveConfig&);`
  - `Vec3 StartSlideVelocity(const Vec3& Velocity, const MoveConfig&);` — добавить `SlideStartBoost` по направлению горизонтальной скорости, клампить к `SlideMaxSpeed`.
  - `Vec3 StepSlide(const Vec3& Velocity, const Vec3& FloorNormal, float Dt, const MoveConfig&);`
  - `bool ShouldExitSlide(const Vec3& Velocity, const MoveConfig&);` — скорость < `SlideExitSpeed`.

- [ ] **Step 1: Тесты:**
  - `ComputeDashVelocity({1,0,0.5f}, {0,1,0}, cfg) == {1500,0,0}`; `ComputeDashVelocity({0,0,0}, {0,1,0}, cfg) == {0,1500,0}`.
  - `TryAirJump` → `true`, второй раз → `false`; после `ResetAirJumps` снова `true`.
  - `CanStartSlide(499, true)` → `false`, `CanStartSlide(600, false)` → `false`, `CanStartSlide(600, true)` → `true`.
  - `StartSlideVelocity({1000,0,0})` → `{1250,0,0}`; `StartSlideVelocity({1700,0,0})` → `{1800,0,0}`.
  - Ровный пол: `StepSlide({1000,0,0}, {0,0,1}, 0.5f)` → `{800,0,0}`.
  - Склон 30° вниз по +X (нормаль `{0.5,0,0.866}`): `Length(StepSlide({1000,0,0}, n, 1.0f))` ≈ 1090 ± 1, и результат лежит в плоскости склона (`|Dot(v,n)| < 1e-2`).
  - `StepSlide` на ровном полу никогда не разворачивает скорость: `StepSlide({100,0,0}, {0,0,1}, 1.0f) == {0,0,0}`.
- [ ] **Step 2:** Запуск → FAIL.
- [ ] **Step 3:** Реализовать. `StepSlide`: повернуть скорость в плоскость пола **с сохранением модуля**; прибавить `Gravity * sin(угол)` по направлению вниз по склону × `Dt`; уменьшить модуль на `SlideDecel * Dt` (не ниже 0); клампить к `SlideMaxSpeed`.
- [ ] **Step 4:** Запуск → PASS.
- [ ] **Step 5:** Commit `feat(sim): dash, air jump and slide rules`.

### Task 4: Математика зиплайна

**Files:** Create `Public/Zipline.h`, `Private/Zipline.cpp`, `SimTests/test_zipline.cpp`

**Interfaces:**
- Produces:
  - `struct ZiplineSegment { Vec3 A, B; };`
  - `float ProjectT(const ZiplineSegment&, const Vec3& Pos);` — параметр ближайшей точки, клампленный к [0,1].
  - `Vec3 PointAt(const ZiplineSegment&, float T);`
  - `float DistanceTo(const ZiplineSegment&, const Vec3& Pos);`
  - `float AdvanceT(const ZiplineSegment&, float T, int Direction /*+1|-1*/, float Speed, float Dt);` — клампит к [0,1].
  - `int ChooseDirection(const ZiplineSegment&, const Vec3& Facing);` — `+1`, если `Dot(Flatten(Facing), Flatten(B−A)) >= 0`.
  - `struct ZiplineConfig { float Speed = 1800.f; float AttachRadius = 200.f; float HangOffsetZ = -120.f; };`

- [ ] **Step 1: Тесты:** отрезок `(0,0,0)–(1000,0,0)`: `ProjectT({250,50,0}) == 0.25`; `ProjectT({-100,0,0}) == 0`; `ProjectT({2000,0,0}) == 1`; `PointAt(0.5) == {500,0,0}`; `DistanceTo({250,50,0}) == 50`; `AdvanceT(0.9, +1, 1800, 0.1) == 1.0`; `AdvanceT(0.5, -1, 1000, 0.1) == 0.4`; `ChooseDirection({-1,0,0}) == -1`; вырожденный отрезок `A == B`: `ProjectT` → 0 без NaN.
- [ ] **Step 2:** Запуск → FAIL.
- [ ] **Step 3:** Реализовать.
- [ ] **Step 4:** Запуск → PASS.
- [ ] **Step 5:** Commit `feat(sim): zipline segment math`.

### Task 5: История хитбоксов

**Files:** Create `Public/HitboxHistory.h`, `Private/HitboxHistory.cpp`, `SimTests/test_hitbox_history.cpp`

**Interfaces:**
- Produces:
  - `enum class HitZone : unsigned char { Body, Head };`
  - `struct Capsule { Vec3 A, B; float Radius; HitZone Zone; };` (сфера — капсула с `A == B`)
  - `class HitboxHistory { public: explicit HitboxHistory(double MaxAgeSeconds = 3.0); void Record(double Time, std::vector<Capsule> Shapes, bool bTeleported = false); std::optional<std::vector<Capsule>> SampleAt(double Time) const; void Prune(double Now); size_t Num() const; };`
- Правила `SampleAt`: время раньше самого старого снимка → `nullopt`; позже самого нового → самый новый; между двумя снимками → покомпонентная интерполяция `A`, `B`, `Radius`; если второй снимок помечен `bTeleported`, интерполяции нет: при `Time < T2` возвращается первый снимок, иначе второй. Если число капсул в соседних снимках разное, возвращается более ранний.

- [ ] **Step 1: Тесты:** запись t=0 (капсула на x=0) и t=1 (x=100): `SampleAt(0.5)` → x=50; `SampleAt(-0.1)` → `nullopt`; `SampleAt(2)` → x=100. Телепорт: t=0 x=0, t=1 x=5000 с `bTeleported` → `SampleAt(0.99)` → x=0, `SampleAt(1.0)` → x=5000. Очистка: записи t=0..5 шагом 1, `Prune(5)` → `Num() == 4` (t=2..5), `SampleAt(1.5)` → `nullopt`. Записи с убывающим временем игнорируются (`Num` не растёт).
- [ ] **Step 2:** Запуск → FAIL.
- [ ] **Step 3:** Реализовать на `std::deque<Snapshot>`.
- [ ] **Step 4:** Запуск → PASS.
- [ ] **Step 5:** Commit `feat(sim): hitbox history with teleport-safe interpolation`.

### Task 6: Луч против капсул и ограничение отката

**Files:** Create `Public/Raycast.h`, `Private/Raycast.cpp`, `SimTests/test_raycast.cpp`

**Interfaces:**
- Consumes: `Capsule`, `HitZone` (Task 5).
- Produces:
  - `struct Ray { Vec3 Origin; Vec3 Dir; };` (`Dir` нормализован)
  - `struct RayHit { float Distance; HitZone Zone; int ShapeIndex; };`
  - `std::optional<RayHit> RaycastCapsules(const Ray&, const std::vector<Capsule>&, float MaxDistance);` — ближайшее попадание; луч изнутри капсулы — попадание с `Distance = 0`.
  - `double ClampRewindTime(double ServerNow, double ClientShotTime, double MaxRewind = 0.25);` — возвращает `ServerNow - clamp(ServerNow - ClientShotTime, 0, MaxRewind)`.

- [ ] **Step 1: Тесты:** тело `A(0,0,0) B(0,0,100) r30 Body`, голова `A=B=(0,0,150) r15 Head`. Луч из `(-500,0,50)` по `+X`: Body, `Distance ≈ 470`. Из `(-500,0,150)`: Head, ≈ 485. Из `(-500,0,200)`: промах. `MaxDistance = 400` при первом луче: промах. Луч по `−X`: промах (цель сзади). `ClampRewindTime(10, 9.9) == 9.9`; `(10, 9.5) == 9.75`; `(10, 10.2) == 10`.
- [ ] **Step 2:** Запуск → FAIL.
- [ ] **Step 3:** Реализовать (луч против отрезка-оси с радиусом: ближайший параметр по сфере-цилиндру; допустимо аналитическое решение для цилиндра плюс две сферы на концах).
- [ ] **Step 4:** Запуск → PASS.
- [ ] **Step 5:** Commit `feat(sim): ray vs capsule and rewind clamp`.

### Task 7: Видимость команды

**Files:** Create `Public/TeamVision.h`, `Private/TeamVision.cpp`, `SimTests/test_team_vision.cpp`

**Interfaces:**
- Produces:
  - `struct VisionActor { int Id; int Team; Vec3 Pos; float SightRadius; bool bObserver; bool bHidden; };`
  - `struct VisionConfig { double GraceSeconds = 1.0; float HiddenRevealRadius = 600.f; };`
  - `using LineOfSightFn = std::function<bool(const VisionActor& Observer, const VisionActor& Target)>;`
  - `class TeamVision { public: explicit TeamVision(VisionConfig = {}); void Update(double Now, const std::vector<VisionActor>&, const LineOfSightFn&); bool IsVisibleToTeam(int TargetId, int Team) const; };`
- Правила: свой для команды всегда виден. Враг виден, если хотя бы один наблюдатель команды (`bObserver`) находится в пределах своего `SightRadius` и у него есть LOS. Скрытый (`bHidden`) враг виден только наблюдателю ближе `HiddenRevealRadius` с LOS. Однажды увиденный враг остаётся видимым `GraceSeconds` (скрытый — без grace). Неизвестный `Id` → `false`.

- [ ] **Step 1: Тесты:** A (team 0, `(0,0,0)`, sight 2000, observer), B (team 1, `(1500,0,0)`). LOS true → B виден team 0. LOS false в `Now=0` → не виден. Увиден в 0, LOS false в 0.5 → виден; в 1.6 → не виден. B скрытый на 1500 → не виден; скрытый на 500 → виден; скрытый, увиден в 0, потом отошёл на 1500 в 0.1 → не виден сразу. B на 2500 → не виден. A виден своей команде всегда. Неизвестный id → `false`.
- [ ] **Step 2:** Запуск → FAIL.
- [ ] **Step 3:** Реализовать (`std::unordered_map<long long, double> LastSeen` по ключу `(TargetId, Team)`).
- [ ] **Step 4:** Запуск → PASS.
- [ ] **Step 5:** Commit `feat(sim): team vision with grace and hidden reveal radius`.

---

> **Задачи 8–14 — UE-обвязка.** Claude пишет код. Автор собирает (`Generate Visual Studio project files` → Build `BrutusEditor Win64 Development`) и проходит ручную проверку из последнего шага задачи. Ошибки компиляции и результаты проверки автор пересылает Claude.

### Task 8: UE-проект, модули, ввод, режим игры

**Files:**
- Create: `Brutus.uproject` (`EngineAssociation` оставить пустым — автор выберет версию через «Switch Unreal Engine version»; модули `Brutus` Runtime и `BrutusSim` Runtime; плагин `GameplayAbilities` включён)
- Create: `Source/Brutus.Target.cs`, `Source/BrutusEditor.Target.cs`, `Source/BrutusServer.Target.cs` (`DefaultBuildSettings = BuildSettingsVersion.Latest`, `IncludeOrderVersion = EngineIncludeOrderVersion.Latest`, ExtraModuleNames `Brutus`, `BrutusSim`)
- Create: `Source/BrutusSim/BrutusSim.Build.cs` (зависимость только `Core`), `Private/BrutusSimModule.cpp` (`IMPLEMENT_MODULE(FDefaultModuleImpl, BrutusSim)`)
- Create: `Source/Brutus/Brutus.Build.cs` (Public: `Core CoreUObject Engine InputCore EnhancedInput GameplayAbilities GameplayTags GameplayTasks NetCore BrutusSim`), `Brutus.h/.cpp` (`IMPLEMENT_PRIMARY_GAME_MODULE`)
- Create: `Core/BrutusSimConvert.h` — `inline bsim::Vec3 ToSim(const FVector&)`, `inline FVector ToUE(const bsim::Vec3&)`
- Create: `Core/BrutusGameMode`, `Core/BrutusGameState`, `Core/BrutusPlayerState` (пока только `UPROPERTY(Replicated) uint8 TeamId`), `Core/BrutusPlayerController`, `Core/BrutusHUD`
- Create: `Config/DefaultEngine.ini` (`GameDefaultMap=/Game/Maps/M_Greybox`, `GlobalDefaultGameMode=/Script/Brutus.BrutusGameMode`), `Config/DefaultGame.ini`, `Config/DefaultInput.ini` (`DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput`, `DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent`)

**Interfaces:**
- Produces:
  - `ABrutusPlayerController` создаёт в рантайме (`NewObject`) `UInputMappingContext` и действия: `IA_Move` (Axis2D, WASD через Swizzle/Negate), `IA_Look` (Axis2D, мышь), `IA_Jump` (Space), `IA_Dash` (LeftShift), `IA_Slide` (LeftControl и C), `IA_Fire` (LMB), `IA_Interact` (E). Публичный геттер `const UInputAction* GetAction(EBrutusInput) const`, `enum class EBrutusInput : uint8 { Move, Look, Jump, Dash, Slide, Fire, Interact }`.
  - `ABrutusGameMode`: `PostLogin` назначает `TeamId` поочерёдно 0/1; `ChoosePlayerStart_Implementation` выбирает `APlayerStart` с тегом `Team0`/`Team1`.

- [ ] **Step 1:** Написать файлы.
- [ ] **Step 2 (автор):** сгенерировать проектные файлы, собрать `BrutusEditor`. Ожидается: сборка без ошибок.
- [ ] **Step 3 (автор):** создать пустую карту `Content/Maps/M_Greybox` (File → New Level → Empty Level), поставить 2 `PlayerStart` с тегами `Team0` и `Team1` и Directional Light. Ожидается: PIE на 2 игроков (Net Mode: Play As Listen Server) — оба появляются у своих стартов, в логе `TeamId` 0 и 1.
- [ ] **Step 4:** Commit `feat: UE project skeleton, runtime input, team assignment`.

### Task 9: Персонаж и движение с предсказанием (стамина, рывок, двойной прыжок, скольжение)

**Files:**
- Create: `Character/BrutusCharacter.h/.cpp`, `Character/BrutusMovementComponent.h/.cpp`
- Modify: `Core/BrutusHUD.cpp` (показ стамины)

**Interfaces:**
- Consumes: `bsim::StaminaConfig/State/TryConsume/Regen` (Task 2), `bsim::MoveConfig`, `ComputeDashVelocity`, `CanStartSlide`, `StartSlideVelocity`, `StepSlide`, `ShouldExitSlide` (Task 3), `EBrutusInput` (Task 8).
- Produces:
  - `enum ECustomMovementMode : uint8 { CMOVE_None = 0, CMOVE_Slide = 1, CMOVE_Zipline = 2 };`
  - `UBrutusMovementComponent : UCharacterMovementComponent` с полями `bsim::StaminaState Stamina`, `bsim::StaminaConfig StaminaCfg`, `bsim::MoveConfig MoveCfg`, флагами ввода `bWantsToDash`, `bWantsToSlide`, `bWantsToZipline`; методами `float GetStaminaCharges() const`, `bool IsSliding() const`, `bool IsOnZipline() const`.
  - Сжатые флаги: `FLAG_Custom_0` — рывок, `FLAG_Custom_1` — скольжение, `FLAG_Custom_2` — зиплайн.
  - `FSavedMove_Brutus` сохраняет флаги и `Stamina.Charges` (`SetMoveFor`/`PrepMoveFor`/`CanCombineWith`: ход с рывком не объединяется).
  - Коррекция стамины сервером: `FBrutusMoveResponseDataContainer : FCharacterMoveResponseDataContainer` с `float StaminaCharges` (`ServerFillResponseData`, `Serialize`), применяется в `ClientHandleMoveResponse`.
  - `ABrutusCharacter`: капсула 42×96, серый меш `/Engine/BasicShapes/Cylinder`, SpringArm 300 с `SocketOffset (0,60,60)`, `bUseControllerRotationYaw = true`, `JumpMaxCount = 2`.
- Правила:
  - Рывок: в `UpdateCharacterStateBeforeMovement`, если `bWantsToDash` и `TryConsume` → `Velocity = ComputeDashVelocity(...)`, режим Falling, если в воздухе.
  - Двойной прыжок: переопределить `DoJump`; если `IsFalling()`, требуется `TryConsume`, иначе `false`.
  - Скольжение: вход из Walking при `bWantsToSlide && CanStartSlide`; `PhysCustom` → `PhysSlide` (`StepSlide` с нормалью `CurrentFloor`, `SafeMoveUpdatedComponent`, проверка пола); выход при `ShouldExitSlide`, отпущенной клавише или потере пола. Капсула в скольжении — высота приседа 50.
  - Восстановление стамины — в `OnMovementUpdated`.

- [ ] **Step 1:** Написать код.
- [ ] **Step 2 (автор):** сборка без ошибок.
- [ ] **Step 3 (автор), ручная проверка:** PIE, 2 клиента, Listen Server, консоль `NetEmulation.PktLag 150`, `NetEmulation.PktLoss 5`:
  - Рывок 3 раза подряд работает, 4-й нет. Стамина на HUD падает до 0 и восстанавливается по 1 заряду за 2.5 с.
  - Двойной прыжок тратит заряд; без зарядов второй прыжок не срабатывает.
  - Скольжение с разбега по ровному замедляется, с горки ускоряется.
  - Спам Shift на нуле стамины 10 с не даёт дёрганья (`p.NetShowCorrections 1` — коррекций почти нет).
  - Второй клиент видит всё это плавно.
- [ ] **Step 4:** Commit `feat: predicted movement — stamina, dash, double jump, slide`.

### Task 10: Зиплайны

**Files:**
- Create: `Movement/BrutusZiplineComponent.h/.cpp` (`USceneComponent`, `UPROPERTY(EditAnywhere) FVector StartLocal, EndLocal`, кабель через `UCableComponent` или растянутый цилиндр), `Movement/BrutusZipline.h/.cpp` (актёр-обёртка), `Movement/BrutusZiplineRegistry.h/.cpp` (`UWorldSubsystem`)
- Modify: `Character/BrutusMovementComponent.*`

**Interfaces:**
- Consumes: `bsim::ZiplineSegment`, `ProjectT`, `PointAt`, `DistanceTo`, `AdvanceT`, `ChooseDirection`, `ZiplineConfig` (Task 4).
- Produces:
  - `UBrutusZiplineRegistry::Register/Unregister(UBrutusZiplineComponent*)`, `UBrutusZiplineComponent* FindNearest(const FVector& Pos, float MaxDist) const`.
  - `bsim::ZiplineSegment UBrutusZiplineComponent::GetWorldSegment() const`.
  - В CMC: `TWeakObjectPtr<UBrutusZiplineComponent> CurrentZipline`, `float ZiplineT`, `int8 ZiplineDir` (сохраняются в `FSavedMove_Brutus`).
- Правила: по флагу зиплайна ищется ближайший в радиусе `AttachRadius` → `CMOVE_Zipline`. `PhysZipline`: позиция = `PointAt(T) + (0,0,HangOffsetZ)`, `T = AdvanceT(...)`. Прыжок → Falling с текущей скоростью + `JumpZVelocity`. Конец троса → Falling. Стамину зиплайн не тратит.

- [ ] **Step 1:** Написать код.
- [ ] **Step 2 (автор):** сборка.
- [ ] **Step 3 (автор), проверка:** поставить `ABrutusZipline` над картой. При пинге 150 мс: E рядом с тросом — цепляется и едет по взгляду, Space — спрыгивает с инерцией, в конце троса падает. Второй клиент видит плавное движение.
- [ ] **Step 4:** Commit `feat: ziplines`.

### Task 11: GAS: здоровье, урон, смерть и возрождение

**Files:**
- Create: `Abilities/BrutusAttributeSet.h/.cpp`, `Abilities/BrutusDamageEffect.h/.cpp`, `Abilities/BrutusGameplayTags.h/.cpp`
- Modify: `Core/BrutusPlayerState.*` (ASC), `Character/BrutusCharacter.*` (`IAbilitySystemInterface`, смерть), `Core/BrutusGameMode.*` (возрождение), `Core/BrutusHUD.cpp` (здоровье)

**Interfaces:**
- Produces:
  - `UBrutusAttributeSet`: `Health`, `MaxHealth` (`ATTRIBUTE_ACCESSORS`, `OnRep_*`, кламп в `PreAttributeChange` и `PostGameplayEffectExecute`). Стартовое значение 250 **[для настройки]**.
  - Нативный тег `Brutus::Tags::Data_Damage` = `"Data.Damage"` (`UE_DEFINE_GAMEPLAY_TAG`).
  - `UBrutusDamageEffect : UGameplayEffect` — Instant, модификатор `Health` Additive через SetByCaller `Data.Damage` (значение передаётся отрицательным).
  - `ABrutusPlayerState`: `UAbilitySystemComponent* AbilitySystem` (Replicated, `EGameplayEffectReplicationMode::Mixed`), `UBrutusAttributeSet* Attributes`.
  - `void ABrutusGameMode::HandleHeroDeath(ABrutusCharacter* Victim, AController* Killer);` — через 5 с **[для настройки]** `RestartPlayer` у командного старта, `Health = MaxHealth`.
  - `DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroTeleported, ABrutusCharacter*)` в `ABrutusCharacter` — вызывается при возрождении и любом телепорте. Его слушает Task 12.
- [ ] **Step 1:** Написать код.
- [ ] **Step 2 (автор):** сборка.
- [ ] **Step 3 (автор), проверка:** консольная команда `brutus.DamageSelf 300` (`FAutoConsoleCommandWithWorldAndArgs`, только сервер или cheat) убивает героя, через 5 с он возрождается с полным здоровьем; здоровье на HUD у владельца верное.
- [ ] **Step 4:** Commit `feat: GAS health, damage effect, death and respawn`.

### Task 12: Компенсация лага и hitscan-оружие

**Files:**
- Create: `Character/BrutusHitboxComponent.h/.cpp`, `Combat/BrutusLagCompensationSubsystem.h/.cpp`, `Combat/BrutusHitscanWeaponComponent.h/.cpp`
- Modify: `Character/BrutusCharacter.*` (компоненты, ввод Fire), `Core/BrutusHUD.cpp` (прицел, маркер попадания)

**Interfaces:**
- Consumes: `bsim::HitboxHistory`, `Capsule`, `HitZone` (Task 5); `RaycastCapsules`, `ClampRewindTime` (Task 6); `UBrutusDamageEffect`, `Data_Damage` (Task 11); `FOnHeroTeleported` (Task 11).
- Produces:
  - `std::vector<bsim::Capsule> UBrutusHitboxComponent::BuildWorldCapsules() const;` — серая модель: тело от низа капсулы до 80% роста, радиус капсулы; голова — сфера r15 на 90% роста.
  - `UBrutusLagCompensationSubsystem : UTickableWorldSubsystem` (тикает только на сервере): каждый тик пишет снимок каждого `ABrutusCharacter`; при `FOnHeroTeleported` пишет снимок с `bTeleported = true`; `Prune` по `HistorySeconds = 3.0`.
  - `struct FBrutusRewindHit { TWeakObjectPtr<ABrutusCharacter> Target; float Distance; bsim::HitZone Zone; FVector ImpactPoint; };`
  - `TOptional<FBrutusRewindHit> RewindTrace(const FVector& Start, const FVector& Dir, float MaxRange, double ShotServerTime, const AActor* Shooter) const;` — проверяет всех, кроме стрелка, на откатанное время; затем трасса `ECC_WorldStatic` от старта до точки попадания: если её перекрыла стена, это промах.
  - `UBrutusHitscanWeaponComponent`: `FireRate = 10` выстр./с, `Damage = 20`, `HeadMultiplier = 1.5`, `MaxRange = 10000` **[для настройки]**. Клиент: трасса от камеры до точки прицела, косметический трейсер сразу, `Server_Fire(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, double ClientServerTime)` (Reliable). Сервер: проверка темпа (`>= 0.8 / FireRate` с прошлого выстрела), `Start` не дальше 200 см от глаз героя, `ClampRewindTime(Now, ClientServerTime, 0.25)`, `RewindTrace`, урон через `UBrutusDamageEffect`, `Client_ConfirmHit(bool bHead)` стрелку, `Multicast_Tracer` (Unreliable) остальным.
  - Время клиента: `GetWorld()->GetGameState()->GetServerWorldTimeSeconds()`.
- [ ] **Step 1:** Написать код.
- [ ] **Step 2 (автор):** сборка.
- [ ] **Step 3 (автор), проверка** при `PktLag 150`: стрельба по бегущему с рывками противнику — попадания засчитываются там, где он был на экране стрелка (`brutus.DebugLagComp 1` рисует откатанные капсулы зелёным, текущие красным). Через стену — промах. Голова даёт ×1.5. Выстрел в героя в момент возрождения не попадает в новую точку.
- [ ] **Step 4:** Commit `feat: lag-compensated hitscan weapon`.

### Task 13: Видимость команды на сервере

**Files:**
- Create: `Vision/BrutusTeamVisionSubsystem.h/.cpp`
- Modify: `Character/BrutusCharacter.*` (`IsNetRelevantFor`, флаг `bHiddenForVision` — для Немо в будущем), `Core/BrutusHUD.cpp` (отладка)

**Interfaces:**
- Consumes: `bsim::TeamVision`, `VisionActor`, `VisionConfig` (Task 7); `ABrutusPlayerState::TeamId` (Task 8).
- Produces:
  - `UBrutusTeamVisionSubsystem : UTickableWorldSubsystem`, сервер, обновление 10 Гц: все `ABrutusCharacter` — наблюдатели с `SightRadius = 3000` **[для настройки]**; LOS — `LineTraceSingleByChannel(ECC_Visibility)` от глаз наблюдателя к голове и к центру цели (любой чистый луч = видно); цель также проверяется с упреждением `Pos + Velocity * 0.1`.
  - `bool IsVisibleToTeam(const AActor* Target, uint8 Team) const;`
  - `ABrutusCharacter::IsNetRelevantFor`: своя команда — `Super`; чужая — `Super && Subsystem->IsVisibleToTeam(this, ViewerTeam)`. Команда зрителя берётся из `PlayerState` контроллера `RealViewer`.
  - Консоль `brutus.DisableVision 1` отключает фильтр (для отладки).
- [ ] **Step 1:** Написать код.
- [ ] **Step 2 (автор):** сборка.
- [ ] **Step 3 (автор), проверка:** 2 клиента в разных командах по разные стороны стены. Враг за стеной не существует на клиенте (`showdebug net` / список актёров). Выход из-за угла — появляется без заметной задержки. Скрылся — исчезает примерно через 1 с.
- [ ] **Step 4:** Commit `feat: server-side team vision and relevancy`.

### Task 14: Серая арена, HUD и приёмка вехи 1

**Files:**
- Create: `Arena/BrutusGreyboxArena.h/.cpp` — в `OnConstruction` создаёт `UStaticMeshComponent` из `/Engine/BasicShapes/Cube`: пол 6000×6000, периметр 400 высотой, 8 укрытий 200×200×150, стена 2000×50×400 по центру с двумя проходами, пандусы 20° и 30° длиной 1500 (для скольжения), площадка высотой 600 и `UBrutusZiplineComponent` от неё к противоположному краю.
- Modify: `Core/BrutusHUD.cpp` — прицел, здоровье, заряды стамины, пинг, число видимых врагов (отладка).

- [ ] **Step 1:** Написать код.
- [ ] **Step 2 (автор):** сборка, поставить `ABrutusGreyboxArena` на `M_Greybox`.
- [ ] **Step 3 (автор), приёмка вехи 1** (2 человека по сети или PIE с `PktLag 100`, `PktLoss 2`): все проверки задач 9–13 проходят на арене; стрельба субъективно честная у обоих игроков; 10 минут без вылетов и ошибок в логе.
- [ ] **Step 4:** Commit `feat: greybox arena and HUD — milestone 1 complete`.
