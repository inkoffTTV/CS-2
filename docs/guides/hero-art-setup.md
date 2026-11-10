# Модели героев в Unreal: пошагово

Игра сама выбирает лучшее, что есть в проекте:
1. **Анимированный герой**: скелетная модель + Animation Blueprint (`ABP_<Герой>`) + оружие в руке.
2. **Статичная модель в позе** (без анимации).
3. **Серые примитивы**, если ничего не импортировано.

## Шаг 1. Импорт моделей (2 минуты)

1. Открой проект в редакторе.
2. Меню **Tools → Execute Python Script…** и выбери файл `Tools/import_hero_art.py` из папки проекта.
3. Дождись окончания. В Content Browser появятся:
   - `Characters/Clavus/` — `SM_Hero_Clavus_Posed`, `SK_Hero_Clavus` (+ скелет, физ. ассет, материалы);
   - `Characters/Malleus/` — то же для Маллеуса;
   - `Weapons/Clavus/SM_Wpn_Clavus_Nailgun`, `Weapons/Malleus/SM_Wpn_Malleus_Hammer`.
4. Нажми **Play**: герои уже показаны статичными моделями.

Если пункта *Execute Python Script* нет: **Edit → Plugins**, найди *Python Editor Script Plugin*, включи, перезапусти редактор.

## Шаг 2. Анимации (10–15 минут, можно позже)

Берём готовые анимации манекена Epic (Manny) и переносим их на наших героев. Названия пунктов меню даны для UE 5.4–5.7; в твоей версии они могут слегка отличаться.

### 2.1. Добавить Manny и его анимации в проект
1. В Content Browser нажми **+ Add** (или **Add**) → **Add Feature or Content Pack…**
2. Вкладка **Blueprint** → **Third Person** → **Add to Project**.
3. Появится папка `Characters/Mannequins/` с моделью `SKM_Manny`, анимациями и Animation Blueprint (`ABP_Manny` или `ABP_Unarmed`, в зависимости от версии).

### 2.2. Перенести анимации на Клавуса
1. Найди в Content Browser этот Animation Blueprint (поиск `ABP_`).
2. Правый клик по нему → **Retarget Animations…** (в некоторых версиях: **Retarget Animation Assets → Duplicate and Retarget**).
3. В открывшемся окне:
   - **Source Skeletal Mesh**: `SKM_Manny`;
   - **Target Skeletal Mesh**: `SK_Hero_Clavus`;
   - **IK Retargeter**: оставь пустым — редактор создаст его сам (кости наших героев названы как у Manny);
   - убедись, что в списке отмечены сам Animation Blueprint и анимации;
   - **Export Path**: `/Game/Characters/Clavus/Anims`, **Suffix**: `_Clavus`.
4. Нажми **Export / Retarget**.
5. Новый Animation Blueprint перемести в `/Game/Characters/Clavus/` и переименуй в **`ABP_Clavus`** (имя важно — игра ищет именно его).

### 2.3. То же для Маллеуса
Повтори 2.2 с `SK_Hero_Malleus`, путём `/Game/Characters/Malleus/Anims`, суффиксом `_Malleus` и именем **`ABP_Malleus`**.

### 2.4. Проверка
**Play**: герои бегают, прыгают и стоят с анимацией, оружие в правой руке.

## Если что-то не так
- Пункт меню называется иначе или его нет — пришли скриншот окна, подскажу по твоей версии.
- Герой повёрнут боком или лежит — пришли скриншот, поправлю поворот в коде (`HeroBody` в `BrutusCharacter.cpp`).
- Оружие висит не в руке или повёрнуто — то же самое, скриншот.
- Руки/ноги выворачивает после переноса анимаций — открой созданный IK Retargeter, вкладка *Edit Retarget Pose*, и поверни руки так, чтобы поза совпала с Manny.
