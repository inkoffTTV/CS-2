# Веха 2: способности и первые герои (Клавус, Маллеус) — план

> Выполняется субагентами по фазам (см. «Исполнение»). Каждая задача — TDD там, где код тестируем нативно (`SimTests/run.sh`); UE-код проверяет автор в редакторе.

**Goal:** На серой арене можно выбрать Клавуса или Маллеуса; у каждого своё оружие, два скилла и ульта, работающие по сети через GAS.

**Architecture:** Математика новых механик (дробь, урон по площади, сферы против капсул, баллистика притяжки) — в `BrutusSim` с нативными тестами. В UE: общий каркас способностей (базовый класс, ввод Q/F/R/ПКМ, кулдауны, оглушение, атрибуты скорости и получаемого урона, снаряды, выбор героя), затем по подклассу персонажа на героя в `Source/Brutus/Heroes/<Hero>/`.

**Tech Stack:** как в вехе 1 (UE 5.x, C++, GAS, Enhanced Input; doctest).

**Spec:** `docs/design.md` (разделы 6, 7, 9 — веха 2); бриф героев — https://claude.ai/artifact/MKeixVQi4t8oiBr266HZiK. База: ветка `m1-movement-shooting`.

## Global Constraints
- Всё из Global Constraints вехи 1 (единицы, `BrutusSim` без UE, сервер решает урон).
- Числа ниже — стартовые для настройки, не требования спеки.
- Ульты в этой вехе работают на длинном кулдауне. Открытие ульты прокачкой за оболы — веха 4.
- Огонь по своим выключен; свои не перекрывают выстрелы (лаг-компенсация пропускает союзников стрелка).
- Модели — серые примитивы; Маллеус крупнее (капсула r 60, полувысота 105), Клавус как базовый (42 / 96).

## Review Focus
1. Скилл нажат при высоком пинге — срабатывает один раз (нет двойного урона от предсказания и подтверждения).
2. Оглушённый герой не стреляет, не двигается и не применяет скиллы — ни на клиенте, ни на сервере.
3. Притяжка цепью цели, стоящей у стены или на зиплайне — цель не застревает в геометрии и слетает с зиплайна.
4. Липкая граната, прилипшая к герою, который умер до взрыва — взрывается на месте смерти, не крашит.
5. Смена героя во время матча — способности прежнего героя пропадают, новые выдаются, кулдауны не переносятся.

---

## Фаза A (параллельно)

### Task A1 [Sonnet]: математика вехи 2 + чистка видимости
**Files:** `Source/BrutusSim/Public|Private/Spread.*`, `AreaDamage.*`, `Overlap.*`, `Ballistics.*`; изменение `TeamVision.*`; тесты `SimTests/test_spread.cpp`, `test_area_damage.cpp`, `test_overlap.cpp`, `test_ballistics.cpp`, дополнение `test_team_vision.cpp`.
**Produces:**
- `std::vector<Vec3> PelletDirections(const Vec3& Forward, float SpreadDegrees, int Count, unsigned Seed);` — детерминирован по `Seed`; каждый результат нормализован и в конусе `SpreadDegrees` от `Forward`; `Count == 1 && Spread == 0` → ровно `Forward`.
- `float RadialFalloff(float Distance, float Radius, float MinFraction);` — 1 в центре, линейно до `MinFraction` на краю, 0 за радиусом.
- `bool SphereOverlapsCapsules(const Vec3& Center, float Radius, const std::vector<Capsule>&);`
- `Vec3 LaunchVelocityToReach(const Vec3& From, const Vec3& To, float Gravity, float FlightTime);` — начальная скорость, при которой точка под гравитацией придёт в `To` за `FlightTime`.
- `TeamVision`: записи `LastSeen` и `Actors` для id, которых нет в текущем `Update`, удаляются (закрывает отложенную мелочь вехи 1).
**Tests:** спред — 100 дробин в конусе, повтор с тем же seed даёт тот же набор, другой seed — другой; falloff 0 → 1, R/2 при Min 0.3 → 0.65, R → 0.3, >R → 0; сфера касается капсулы/не касается; баллистика — подстановка в `From + V*t + 0.5*g*t²` даёт `To` (±1 см); пропавший из `Update` id → `IsVisibleToTeam == false`, и при повторном появлении grace не наследуется.

### Task A2 [Sonnet]: отложенные мелочи вехи 1 (UE)
**Files:** `Character/BrutusCharacter.cpp`, `Character/BrutusMovementComponent.cpp/.h`, `Arena/BrutusGreyboxArena.h`, `Combat/BrutusLagCompensationSubsystem.cpp`.
- `#include "Engine/StaticMesh.h"` в `BrutusCharacter.cpp`, `class UStaticMesh;` в `BrutusGreyboxArena.h`.
- Прыжок в воздухе тратит заряд только после успешного `Super::DoJump` (сначала проверка `Stamina.Charges >= 1`).
- После окончания рывка в воздухе горизонтальная скорость клампится к `MaxWalkSpeed * 1.25` (один раз, в момент `DashTimeRemaining` → 0).
- `BodyMesh` крепится к `GetMesh()` (сглаживание сетевого движения), с поправкой смещения при приседе.
- `ClientHandleMoveResponse`: стамина/рывок из коррекции применяются, только если `Super` принял коррекцию (сравнить `ClientData->LastAckedMove`/`bUpdatePosition` до и после).
- `RewindTrace`: пропускать героев той же команды, что стрелок.

## Фаза B (после A2)

### Task B1 [Opus]: каркас способностей и выбор героя
**Files:** `Abilities/BrutusGameplayAbility.*`, `Abilities/BrutusCooldownEffect.*`, `Abilities/BrutusDamageExecution.*`, `Abilities/BrutusGameplayTags.*`, `Abilities/BrutusAttributeSet.*` (+`MoveSpeedMultiplier`, `DamageTakenMultiplier`), `Abilities/BrutusDamageEffect.*` (через execution), `Combat/BrutusProjectile.*`, `Combat/BrutusHitscanWeaponComponent.*` (дробь и разброс), `Core/BrutusPlayerController.*` (Q, F, R, ПКМ), `Core/BrutusPlayerState.*` (`SelectedHero`), `Core/BrutusGameMode.*` (класс пешки по герою), `Character/BrutusCharacter.*` (выдача способностей, ввод, оглушение), `Character/BrutusMovementComponent.*` (множитель скорости, оглушение), `Heroes/BrutusHeroRegistry.*`.
**Produces:**
- `enum class EBrutusAbilityInput : uint8 { None, Ability1 /*Q*/, Ability2 /*F*/, Ultimate /*R*/, AltFire /*ПКМ*/ };`
- `UBrutusGameplayAbility : UGameplayAbility` — `InputId`, `CooldownSeconds`, `NetExecutionPolicy = LocalPredicted`; кулдаун через `UBrutusCooldownEffect` (SetByCaller `Cooldown` длительность + динамический тег `Cooldown.<AbilityName>`); блокируется тегом `State.Stunned`.
- Нативные теги: `State.Stunned`, `State.Dead`, `Cooldown.*` (динамические через `FGameplayTag::RequestGameplayTag` из ini), `Event.Hit`.
- `UBrutusStunEffect` (HasDuration, SetByCaller `Duration`, выдаёт `State.Stunned`). CMC: при `State.Stunned` — нулевое ускорение и запрет прыжка/рывка; оружие не стреляет.
- `UBrutusDamageExecution` — урон × `DamageTakenMultiplier` цели; `UBrutusDamageEffect::Apply` прежней сигнатуры.
- `ABrutusProjectile` — реплицируемый, `UProjectileMovementComponent`, виртуальные `OnImpact(const FHitResult&)`, `Explode()`; спавнит только сервер.
- Оружие: параметры `Pellets`, `SpreadDegrees`; клиент шлёт `Seed`, сервер восстанавливает дробь через `bsim::PelletDirections`; урон суммируется по попаданиям.
- `enum class EBrutusHero : uint8 { Clavus, Malleus };` реестр `TSubclassOf<ABrutusCharacter> GetHeroClass(EBrutusHero)`; `ABrutusPlayerState::SelectedHero` (Replicated); `ABrutusGameMode::GetDefaultPawnClassForController_Implementation`; консоль `brutus.Hero Clavus|Malleus` → Server RPC контроллера → смена героя с мгновенным возрождением; способности выдаются в `PossessedBy` (сначала `ClearAllAbilities`).
- `ABrutusCharacter` получает виртуальные `GetHeroAbilities()` и `ConfigureWeapon()` для подклассов.

## Фаза C (после B1, параллельно)

### Task C1 [Opus]: Клавус — `Heroes/Clavus/`
HP 275. Гвоздомёт: темп 12, урон 14, разброс 1.2°, голова ×1.5.
- **Q Липкая граната** (кулдаун 12 с): снаряд, прилипает к первому попаданию (к герою — аттачится), взрыв через 2 с, урон 90, радиус 350, `RadialFalloff` min 0.3, по всем врагам в радиусе с проверкой прямой видимости до центра. Если носитель умер — отцепляется и взрывается на месте.
- **F Щит** (20 с): стена 300×200×20 перед героем на земле, 400 HP, блокирует выстрелы и снаряды (объектный тип WorldStatic), исчезает через 8 с или при 0 HP.
- **R Таран** (60 с): рывок 1200 см/с вперёд 0.6 с (root motion task), враги на пути — 60 урона, отброс, оглушение 0.5 с, каждый не больше раза за применение.

### Task C2 [Opus]: Маллеус — `Heroes/Malleus/`
HP 400, капсула 60/105. Дробовик: 8 дробин × 9, разброс 5°, темп 1.4. **ПКМ удар молотом** (0.9 с): сфера r 120 в 150 см перед героем по откатанным хитбоксам (`SphereOverlapsCapsules`), 75 урона.
- **Q Прыжок с ударом** (14 с): подскок (Z 900), при приземлении — урон 70 в радиусе 400 (`RadialFalloff` min 0.5) и замедление 30% на 2 с (`MoveSpeedMultiplier`).
- **F Цепь** (16 с): трасса 1500 см; первый враг получает 40 урона и притягивается к точке в 150 см перед Маллеусом (`LaunchVelocityToReach`, время полёта 0.45 с); снимается с зиплайна/скольжения.
- **R Ярость** (75 с): 10 с — масштаб ×1.3 (меш и капсула, реплицируется), лечение 300, `DamageTakenMultiplier` 0.7.

## Фаза D
### Task D1 [Opus]: финальное ревью ветки, затем один проход исправлений.

## Исполнение
| Задача | Модель | Почему |
|---|---|---|
| A1 | Sonnet | чистая математика с тестами, чётко специфицирована |
| A2 | Sonnet | мелкие механические правки |
| B1 | Opus | GAS + сеть + предсказание, высокая цена ошибки, никто не компилирует |
| C1, C2 | Opus | сетевые способности, взаимодействие с движением и лаг-компенсацией |
| D1 | Opus | ревью — задача на суждение |
Каждый субагент работает в своей git-копии (worktree) и коммитит в свою ветку; оркестратор сливает ветки в `m2-heroes`, гоняет `SimTests/run.sh` и отправляет на GitHub.
