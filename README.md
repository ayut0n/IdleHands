# IdleHands 🖐️
![C++](https://img.shields.io/badge/C++-17-blue.svg)
![ImGui](https://img.shields.io/badge/UI-Dear%20ImGui-red)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)

**[English](#english)** | **[Русский](#русский)**

---

<a id="english"></a>
## 🇬🇧 English

**IdleHands** is a lightweight, modern, and highly customizable Steam launcher built with C++ and Dear ImGui. It completely bypasses the official 5-account limit and the annoying "Who's playing?" screen introduced in the recent Steam UI updates.

### ✨ Features
* **Limitless Accounts:** Automatically parses your `loginusers.vdf` and allows one-click login to any saved account.
* **Smart UI Bypass:** Edits Steam configuration files on the fly to bypass the "Who's playing?" screen without asking for a password.
* **Direct Launch:** Launch a specific account and directly boot into a game (e.g., AppID `730` for CS2) while keeping the Steam client hidden in the background.
* **Discord Rich Presence:** Automatically updates your Discord status to show which account you are currently playing on.
* **Invisible Mode:** Built-in option to launch Steam accounts directly into "Invisible" status.
* **Modern Design:** Borderless window, smooth rounded corners, customizable accent colors, and automatic avatar parsing from Steam's cache.
* **System Tray Integration:** Runs quietly in the background and can be minimized to the system tray.

### 🚀 Usage
1. Download the latest release from the [Releases](../../releases) tab.
2. Unpack the archive (make sure `discord-rpc.dll` is in the same folder as the `.exe`).
3. Run `IdleHands.exe`.
> **Note:** For the auto-login to work, you must have logged into your Steam accounts at least once with the "Remember Password" box checked.

### 🛠️ Building from source
**Requirements:** VS Code, CMake, MSYS2 (MinGW64).
1. Clone the repository.
2. Install GLFW via MSYS2: `pacman -S mingw-w64-x86_64-glfw`.
3. Include [Dear ImGui](https://github.com/ocornut/imgui), [stb_image](https://github.com/nothings/stb), and [Discord RPC](https://github.com/discordapp/discord-rpc).
4. Build using CMake.

---

<a id="русский"></a>
## 🇷🇺 Русский

**IdleHands** — это легковесный, современный и настраиваемый лаунчер для Steam, написанный на C++ с использованием Dear ImGui. Программа полностью обходит официальный лимит в 5 аккаунтов и избавляет от надоедливого окна «Кто играет?», добавленного в новых обновлениях Steam.

### ✨ Главные возможности
* **Безлимитные аккаунты:** Автоматически считывает файл `loginusers.vdf` и позволяет заходить в любой сохраненный аккаунт в один клик.
* **Умный обход интерфейса:** На лету редактирует файлы конфигурации Steam, чтобы обойти экран «Кто играет?» без повторного ввода пароля.
* **Direct Launch:** Запуск аккаунта сразу с нужной игрой (например, AppID `730` для CS2), при этом сам клиент Steam запускается в скрытом (тихом) режиме.
* **Discord Rich Presence:** Транслирует ваш текущий аккаунт прямо в статус Discord.
* **Режим невидимки:** Возможность заходить на аккаунты сразу со статусом «Невидимка».
* **Современный дизайн:** Окно без рамок с плавными закруглениями, настраиваемый цвет кнопок и автоматическая подгрузка ваших аватарок из кэша Steam.
* **Сворачивание в трей:** Программа не мешается на панели задач и может работать в фоновом режиме.

### 🚀 Использование
1. Скачайте последнюю версию из вкладки [Releases](../../releases).
2. Распакуйте архив (убедитесь, что файл `discord-rpc.dll` лежит в одной папке с `.exe`).
3. Запустите `IdleHands.exe`.
> **Важно:** Чтобы авто-вход работал, вы должны хотя бы один раз войти в каждый аккаунт через официальный клиент Steam с установленной галочкой «Запомнить пароль».

### 🛠️ Сборка из исходников
**Требования:** VS Code, CMake, MSYS2 (MinGW64).
1. Клонируйте репозиторий.
2. Установите GLFW через MSYS2: `pacman -S mingw-w64-x86_64-glfw`.
3. Скачайте в папку проекта библиотеки [Dear ImGui](https://github.com/ocornut/imgui), [stb_image](https://github.com/nothings/stb) и [Discord RPC](https://github.com/discordapp/discord-rpc).
4. Соберите проект с помощью CMake.

---
*Disclaimer: This project is not affiliated with Valve Corporation. Steam is a registered trademark of Valve Corporation.*
