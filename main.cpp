#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Поддержка нативных функций Windows
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>
#include <cctype>
#include <ctime>

// Подключение библиотеки для загрузки картинок
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Подключение Discord RPC
#include "discord_rpc.h"

// Структура аккаунта Steam
struct SteamAccount {
    std::string steamID64;
    std::string accountName;
    std::string personaName;
    GLuint avatarTex = 0; 
};

// Глобальные настройки
float accentColor[3] = { 0.2f, 0.6f, 1.0f };
bool isEnglish = false;
bool invisibleStart = false;
std::vector<SteamAccount> accounts;

// --- ТРЕЙ ---
#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATA nid = {};
WNDPROC originalWndProc = nullptr;

LRESULT CALLBACK WindowProcHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_TRAYICON) {
        if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            GLFWwindow* currentWindow = glfwGetCurrentContext();
            if (currentWindow) {
                glfwShowWindow(currentWindow);
                glfwRestoreWindow(currentWindow);
            }
        }
    }
    return CallWindowProc(originalWndProc, hwnd, uMsg, wParam, lParam);
}
// -------------

// --- DISCORD RPC ---
const char* DISCORD_CLIENT_ID = "123456789012345678"; // ЗАМЕНИТЕ НА СВОЙ ID ИЗ DISCORD DEVELOPER PORTAL

void InitDiscord() {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(DISCORD_CLIENT_ID, &handlers, 1, NULL);
}

void UpdateDiscordPresence(const std::string& state, const std::string& details) {
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    
    discordPresence.state = state.c_str();
    if (!details.empty()) {
        discordPresence.details = details.c_str();
    }
    
    Discord_UpdatePresence(&discordPresence);
}
// -------------------

void SaveConfig() {
    std::ofstream file("idlehands_config.txt");
    if (file.is_open()) {
        file << accentColor[0] << " " << accentColor[1] << " " << accentColor[2] << "\n";
        file << isEnglish << "\n";
        file << invisibleStart << "\n";
    }
}

void LoadConfig() {
    std::ifstream file("idlehands_config.txt");
    if (file.is_open()) {
        file >> accentColor[0] >> accentColor[1] >> accentColor[2];
        file >> isEnglish;
        file >> invisibleStart;
    }
}

bool LoadTextureFromFile(const char* filename, GLuint* out_texture) {
    int image_width = 0, image_height = 0, channels = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 4);
    if (image_data == NULL) return false;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    return true;
}

std::string GetSteamDirectory() {
    char buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "SteamPath", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }
    return "C:/Program Files (x86)/Steam"; 
}

void LoadSteamAccounts() {
    for (auto& acc : accounts) {
        if (acc.avatarTex) glDeleteTextures(1, &acc.avatarTex);
    }
    accounts.clear();

    std::string steamDir = GetSteamDirectory();
    std::string vdfPath = steamDir + "/config/loginusers.vdf";
    std::ifstream file(vdfPath);
    if (!file.is_open()) return;

    std::string line;
    std::regex idRegex("\"(\\d{17})\"");
    std::regex accRegex("\"AccountName\"\\s+\"([^\"]+)\"");
    std::regex personaRegex("\"PersonaName\"\\s+\"([^\"]+)\"");

    SteamAccount currentAcc;
    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, idRegex)) {
            if (!currentAcc.steamID64.empty()) accounts.push_back(currentAcc);
            currentAcc = SteamAccount();
            currentAcc.steamID64 = match[1];
        }
        else if (std::regex_search(line, match, accRegex)) {
            currentAcc.accountName = match[1];
        }
        else if (std::regex_search(line, match, personaRegex)) {
            currentAcc.personaName = match[1];
        }
    }
    if (!currentAcc.steamID64.empty()) accounts.push_back(currentAcc);

    for (auto& acc : accounts) {
        std::string baseAvatar = steamDir + "/config/avatarcache/" + acc.steamID64;
        GLuint tex = 0;
        if (LoadTextureFromFile((baseAvatar + ".png").c_str(), &tex) ||
            LoadTextureFromFile((baseAvatar + ".jpg").c_str(), &tex) ||
            LoadTextureFromFile((baseAvatar + "_full.png").c_str(), &tex) ||
            LoadTextureFromFile((baseAvatar + "_full.jpg").c_str(), &tex)) {
            acc.avatarTex = tex;
        }
    }
}

void KillSteam() {
    bool steamIsAlive = true;
    int attempts = 0;
    while (steamIsAlive && attempts < 10) {
        steamIsAlive = false;
        HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
        PROCESSENTRY32 pEntry;
        pEntry.dwSize = sizeof(pEntry);
        BOOL hRes = Process32First(hSnapShot, &pEntry);
        
        while (hRes) {
            if (_stricmp(pEntry.szExeFile, "steam.exe") == 0 || 
                _stricmp(pEntry.szExeFile, "steamwebhelper.exe") == 0 ||
                _stricmp(pEntry.szExeFile, "steamservice.exe") == 0 ||
                _stricmp(pEntry.szExeFile, "steamerrorreporter.exe") == 0) {
                
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 9);
                    CloseHandle(hProcess);
                }
                steamIsAlive = true;
            }
            hRes = Process32Next(hSnapShot, &pEntry);
        }
        CloseHandle(hSnapShot);
        if (steamIsAlive) std::this_thread::sleep_for(std::chrono::milliseconds(200));
        attempts++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
}

void LaunchSteam(const SteamAccount& targetAcc, const std::string& appID = "", bool forceSilent = false) {
    KillSteam();

    std::string discordState = isEnglish ? "Playing on account" : "Играет на аккаунте";
    UpdateDiscordPresence(discordState, targetAcc.personaName);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "AutoLoginUser", 0, REG_SZ, (const BYTE*)targetAcc.accountName.c_str(), targetAcc.accountName.length() + 1);
        DWORD rememberPwd = 1;
        RegSetValueExA(hKey, "RememberPassword", 0, REG_DWORD, (const BYTE*)&rememberPwd, sizeof(rememberPwd));
        RegCloseKey(hKey);
    }

    std::string configPath = GetSteamDirectory() + "/config/config.vdf";
    SetFileAttributesA(configPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    std::ifstream cfgIn(configPath);
    if (cfgIn.is_open()) {
        std::vector<std::string> cfgLines;
        std::string cfgLine;
        while (std::getline(cfgIn, cfgLine)) {
            if (!cfgLine.empty() && cfgLine.back() == '\r') cfgLine.pop_back();
            std::string lowerCfg = cfgLine;
            std::transform(lowerCfg.begin(), lowerCfg.end(), lowerCfg.begin(), [](unsigned char c){ return std::tolower(c); });
            
            if (lowerCfg.find("\"startupshowaccountpicker\"") != std::string::npos) {
                cfgLine = "\t\t\t\t\t\"StartupShowAccountPicker\"\t\t\"0\"";
            }
            else if (lowerCfg.find("\"autologinuser\"") != std::string::npos) {
                size_t firstNonSpace = cfgLine.find_first_not_of(" \t");
                std::string indent = (firstNonSpace != std::string::npos) ? cfgLine.substr(0, firstNonSpace) : "\t\t\t\t\t";
                cfgLine = indent + "\"AutoLoginUser\"\t\t\"" + targetAcc.accountName + "\"";
            }
            else if (lowerCfg.find("\"rememberpassword\"") != std::string::npos) {
                size_t firstNonSpace = cfgLine.find_first_not_of(" \t");
                std::string indent = (firstNonSpace != std::string::npos) ? cfgLine.substr(0, firstNonSpace) : "\t\t\t\t\t";
                cfgLine = indent + "\"RememberPassword\"\t\t\"1\"";
            }
            cfgLines.push_back(cfgLine);
        }
        cfgIn.close();
        std::ofstream cfgOut(configPath);
        for (const auto& l : cfgLines) cfgOut << l << "\n";
        cfgOut.close();
    }

    std::string vdfPath = GetSteamDirectory() + "/config/loginusers.vdf";
    SetFileAttributesA(vdfPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    
    std::ifstream inFile(vdfPath);
    std::vector<std::string> lines;
    std::string line;
    if (inFile.is_open()) {
        while (std::getline(inFile, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
        inFile.close();
    }

    std::vector<std::string> newLines;
    std::string currentID = "";
    int depth = 0;
    bool targetFoundMostRecent = false, targetFoundAutoLogin = false, targetFoundRemember = false, targetFoundTimestamp = false;
    std::regex rxID("\"(\\d{17})\"");

    for (const auto& l : lines) {
        std::string trimmed = l;
        if (!trimmed.empty()) {
            size_t firstNonSpace = trimmed.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos) trimmed.erase(0, firstNonSpace);
            else trimmed = "";
        }
        std::string lowerTrimmed = trimmed;
        std::transform(lowerTrimmed.begin(), lowerTrimmed.end(), lowerTrimmed.begin(), [](unsigned char c){ return std::tolower(c); });

        if (trimmed == "{") depth++;
        else if (trimmed == "}") {
            if (depth == 2 && currentID == targetAcc.steamID64) {
                if (!targetFoundMostRecent) newLines.push_back("\t\t\"MostRecent\"\t\t\"1\"");
                if (!targetFoundAutoLogin) newLines.push_back("\t\t\"AllowAutoLogin\"\t\t\"1\"");
                if (!targetFoundRemember) newLines.push_back("\t\t\"RememberPassword\"\t\t\"1\"");
                if (!targetFoundTimestamp) newLines.push_back("\t\t\"Timestamp\"\t\t\"" + std::to_string(std::time(nullptr)) + "\"");
            }
            depth--;
        }

        std::smatch match;
        if (depth == 1 && std::regex_search(trimmed, match, rxID)) {
            currentID = match[1];
            if (currentID == targetAcc.steamID64) targetFoundMostRecent = targetFoundAutoLogin = targetFoundRemember = targetFoundTimestamp = false;
        }

        if (depth == 2) {
            if (lowerTrimmed.find("\"mostrecent\"") == 0) {
                newLines.push_back(currentID == targetAcc.steamID64 ? "\t\t\"MostRecent\"\t\t\"1\"" : "\t\t\"MostRecent\"\t\t\"0\"");
                if (currentID == targetAcc.steamID64) targetFoundMostRecent = true;
                continue; 
            }
            if (lowerTrimmed.find("\"allowautologin\"") == 0) {
                newLines.push_back(currentID == targetAcc.steamID64 ? "\t\t\"AllowAutoLogin\"\t\t\"1\"" : "\t\t\"AllowAutoLogin\"\t\t\"0\"");
                if (currentID == targetAcc.steamID64) targetFoundAutoLogin = true;
                continue;
            }
            if (lowerTrimmed.find("\"rememberpassword\"") == 0) {
                if (currentID == targetAcc.steamID64) { newLines.push_back("\t\t\"RememberPassword\"\t\t\"1\""); targetFoundRemember = true; } 
                else newLines.push_back(l);
                continue;
            }
            if (lowerTrimmed.find("\"timestamp\"") == 0) {
                if (currentID == targetAcc.steamID64) { newLines.push_back("\t\t\"Timestamp\"\t\t\"" + std::to_string(std::time(nullptr)) + "\""); targetFoundTimestamp = true; } 
                else newLines.push_back(l);
                continue;
            }
        }
        newLines.push_back(l); 
    }

    std::ofstream outFile(vdfPath);
    for (const auto& nl : newLines) outFile << nl << "\n";
    outFile.close();

    std::string steamExePath = GetSteamDirectory() + "/steam.exe";
    std::string params = "";
    if (forceSilent) params += "-silent ";
    if (!appID.empty()) params += "-applaunch " + appID + " ";
    if (invisibleStart) params += "steam://friends/status/invisible ";

    ShellExecuteA(NULL, "open", steamExePath.c_str(), params.c_str(), NULL, SW_SHOWNORMAL);
}

void DrawSpinner(float radius, float thickness, ImU32 color) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.x += radius; pos.y += radius;
    
    float time = (float)ImGui::GetTime();
    float start_angle = time * 4.0f;
    float end_angle = start_angle + 4.71238f;
    
    drawList->PathArcTo(pos, radius, start_angle, end_angle, 40);
    drawList->PathStroke(color, 0, thickness);
    
    ImVec2 dotPos(pos.x + cosf(end_angle) * radius, pos.y + sinf(end_angle) * radius);
    drawList->AddCircleFilled(dotPos, thickness * 1.2f, color, 12);
}

int main() {
    if (!glfwInit()) return -1;
    
    HWND consoleWnd = GetConsoleWindow();
    if (consoleWnd != NULL) ShowWindow(consoleWnd, SW_HIDE);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "IdleHands", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    HWND hwnd = glfwGetWin32Window(window);
    originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProcHook);

    // Усиленная загрузка иконок с принудительным качественным сглаживанием (LR_CREATEDIBSECTION)
    HICON hIconBig = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(1), IMAGE_ICON, 
                                       GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 
                                       LR_SHARED | LR_CREATEDIBSECTION);
                                       
    HICON hIconSmall = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(1), IMAGE_ICON, 
                                         GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 
                                         LR_SHARED | LR_CREATEDIBSECTION);

    // Запасной вариант, если в resource.rc написано IDI_ICON1 вместо 1
    if (!hIconBig) hIconBig = (HICON)LoadImageA(GetModuleHandle(NULL), "IDI_ICON1", IMAGE_ICON, 
                                                GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 
                                                LR_SHARED | LR_CREATEDIBSECTION);
                                                
    if (!hIconSmall) hIconSmall = (HICON)LoadImageA(GetModuleHandle(NULL), "IDI_ICON1", IMAGE_ICON, 
                                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 
                                                    LR_SHARED | LR_CREATEDIBSECTION);

    // Устанавливаем иконки для окна (Alt+Tab и панель задач)
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    // Устанавливаем иконку для трея
    nid.hIcon = hIconSmall ? hIconSmall : LoadIcon(NULL, IDI_APPLICATION);
    strcpy(nid.szTip, "IdleHands");
    Shell_NotifyIcon(NIM_ADD, &nid);

    InitDiscord();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f, &font_config, io.Fonts->GetGlyphRangesCyrillic());

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 12.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameRounding = 6.0f; 
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.14f, 1.00f);
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    LoadConfig(); 
    LoadSteamAccounts();
    UpdateDiscordPresence(isEnglish ? "Choosing an account" : "Выбирает аккаунт", "");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(2000, 5000);
    auto startTime = std::chrono::steady_clock::now();
    int loadDurationMs = distr(gen);

    char dlAppIDBuffer[16] = ""; 

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Discord_RunCallbacks(); 

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        style.Colors[ImGuiCol_Button] = ImVec4(accentColor[0], accentColor[1], accentColor[2], 0.8f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(accentColor[0], accentColor[1], accentColor[2], 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(accentColor[0] * 0.8f, accentColor[1] * 0.8f, accentColor[2] * 0.8f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(accentColor[0] * 0.4f, accentColor[1] * 0.4f, accentColor[2] * 0.4f, 0.8f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(accentColor[0] * 0.8f, accentColor[1] * 0.8f, accentColor[2] * 0.8f, 1.0f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(accentColor[0], accentColor[1], accentColor[2], 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(accentColor[0] * 0.2f, accentColor[1] * 0.2f, accentColor[2] * 0.2f, 0.8f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(accentColor[0] * 0.4f, accentColor[1] * 0.4f, accentColor[2] * 0.4f, 1.0f);

        auto currentTime = std::chrono::steady_clock::now();
        float elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        float loadingAlpha = 1.0f;
        bool showLoading = true;
        bool showMain = false;

        if (elapsedMs < loadDurationMs) {
            loadingAlpha = 1.0f;
            showLoading = true;
            showMain = false;
        } else if (elapsedMs < loadDurationMs + 800) { 
            loadingAlpha = 1.0f - ((elapsedMs - loadDurationMs) / 800.0f);
            showLoading = true;
            showMain = true; 
        } else {
            loadingAlpha = 0.0f;
            showLoading = false;
            showMain = true;
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

        // --- КАСТОМНЫЙ ЗАГОЛОВОК ОКНА ---
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("DragArea", ImVec2(io.DisplaySize.x - 70, 30));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ReleaseCapture();
            SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); 
        }

        ImGui::SetCursorPos(ImVec2(15, 6));
        ImGui::Text("IdleHands");

        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - 65, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

        if (ImGui::Button("_", ImVec2(30, 30))) {
            glfwHideWindow(window);
        }
        ImGui::SameLine(0, 0);
        if (ImGui::Button("X", ImVec2(30, 30))) glfwSetWindowShouldClose(window, GLFW_TRUE);
        ImGui::PopStyleColor(3);

        ImGui::SetCursorPos(ImVec2(0, 30));
        ImGui::Separator();
        // --------------------------------

        if (showMain) {
            ImGui::SetCursorPos(ImVec2(8, 40));
            if (ImGui::BeginTabBar("MainTabs")) {
                if (ImGui::BeginTabItem(isEnglish ? "Accounts" : "Аккаунты")) {
                    ImGui::Text(isEnglish ? "Select an account to login:" : "Выберите аккаунт для входа:");
                    ImGui::Separator();
                    for (const auto& acc : accounts) {
                        ImGui::PushID(("acc_" + acc.steamID64).c_str());
                        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                        ImVec2 btnSize(ImGui::GetContentRegionAvail().x, 48); 
                        
                        if (ImGui::Button("##acc_btn", btnSize)) LaunchSteam(acc, "", false); 
                        
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        if (acc.avatarTex) {
                            drawList->AddImageRounded((void*)(intptr_t)acc.avatarTex, 
                                                      ImVec2(cursorPos.x + 8, cursorPos.y + 8), ImVec2(cursorPos.x + 40, cursorPos.y + 40), 
                                                      ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 6.0f);
                        } else {
                            drawList->AddRectFilled(ImVec2(cursorPos.x + 8, cursorPos.y + 8), ImVec2(cursorPos.x + 40, cursorPos.y + 40), IM_COL32(80, 80, 80, 255), 6.0f);
                        }
                        
                        std::string label = acc.personaName + " (" + acc.accountName + ")";
                        drawList->AddText(ImVec2(cursorPos.x + 50, cursorPos.y + 15), ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
                        ImGui::PopID();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(isEnglish ? "Direct Launch" : "Direct Launch")) {
                    ImGui::Text(isEnglish ? "Launch account and game instantly" : "Запуск аккаунта сразу с игрой");
                    ImGui::InputText("AppID", dlAppIDBuffer, IM_ARRAYSIZE(dlAppIDBuffer));
                    ImGui::Separator();
                    for (const auto& acc : accounts) {
                        ImGui::PushID(("dl_" + acc.steamID64).c_str());
                        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                        ImVec2 btnSize(ImGui::GetContentRegionAvail().x, 48);
                        
                        if (ImGui::Button("##dl_btn", btnSize)) LaunchSteam(acc, dlAppIDBuffer, true); 
                        
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        if (acc.avatarTex) {
                            drawList->AddImageRounded((void*)(intptr_t)acc.avatarTex, 
                                                      ImVec2(cursorPos.x + 8, cursorPos.y + 8), ImVec2(cursorPos.x + 40, cursorPos.y + 40), 
                                                      ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 6.0f);
                        } else {
                            drawList->AddRectFilled(ImVec2(cursorPos.x + 8, cursorPos.y + 8), ImVec2(cursorPos.x + 40, cursorPos.y + 40), IM_COL32(80, 80, 80, 255), 6.0f);
                        }
                        
                        std::string label = acc.personaName + " (" + acc.accountName + ")";
                        drawList->AddText(ImVec2(cursorPos.x + 50, cursorPos.y + 15), ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
                        ImGui::PopID();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(isEnglish ? "Settings" : "Настройки")) {
                    if (ImGui::ColorEdit3(isEnglish ? "Accent Color" : "Цвет акцента", accentColor, ImGuiColorEditFlags_NoInputs)) SaveConfig(); 
                    if (ImGui::Checkbox(isEnglish ? "English Language" : "Английский язык", &isEnglish)) {
                        SaveConfig();
                        UpdateDiscordPresence(isEnglish ? "Choosing an account" : "Выбирает аккаунт", "");
                    }
                    if (ImGui::Checkbox(isEnglish ? "Invisible Status on Login" : "Входить со статусом «Невидимка»", &invisibleStart)) SaveConfig();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }

        if (showLoading) {
            ImDrawList* overlayDrawList = ImGui::GetWindowDrawList();
            ImVec2 winPos = ImGui::GetWindowPos();
            
            overlayDrawList->AddRectFilled(
                ImVec2(winPos.x, winPos.y + 31), 
                ImVec2(winPos.x + io.DisplaySize.x, winPos.y + io.DisplaySize.y), 
                IM_COL32(28, 28, 36, (int)(loadingAlpha * 255))
            );

            ImGui::SetCursorPos(ImVec2(io.DisplaySize.x / 2 - 30, io.DisplaySize.y / 2 - 30));
            DrawSpinner(30.0f, 4.0f, IM_COL32(255, 255, 255, (int)(loadingAlpha * 255)));

            ImGui::SetCursorPos(ImVec2(io.DisplaySize.x / 2 - 35, io.DisplaySize.y / 2 + 40));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, loadingAlpha);
            ImGui::Text(isEnglish ? "Loading..." : "Загрузка...");
            ImGui::PopStyleVar();

            ImGui::SetCursorPos(ImVec2(0, 31));
            ImGui::InvisibleButton("LoadingBlocker", ImVec2(io.DisplaySize.x, io.DisplaySize.y - 31));
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    Discord_Shutdown();
    for (auto& acc : accounts) if (acc.avatarTex) glDeleteTextures(1, &acc.avatarTex);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}