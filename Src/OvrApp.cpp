#include <VrApi/Include/VrApi_Types.h>
#include "OvrApp.h"
#include "MenuHelper.h"

using namespace OVR;

#define STR_HEADER "GearboyVR"
#define STR_VERSION "ver.1.1"

#define HEADER_HEIGHT 75
#define BOTTOM_HEIGHT 30
#define menuWidth 640
#define menuHeight 576

#define MAX_SAVE_SLOTS 10

#define MoveSpeed 0.00390625f
#define ZoomSpeed 0.03125f
#define MIN_DISTANCE 0.5f
#define MAX_DISTANCE 5.5f

#define OPEN_MENU_SPEED 0.1f
#define MENU_TRANSITION_SPEED 0.1f

#define GL(func) func;

const int SAVE_FILE_VERSION = 4;

std::vector<Emulator::Rom> *romFileList = new std::vector<Emulator::Rom>();

int menuItemSize = 10;
bool menuOpen = true;
bool loadedRom = false;

// saved variables
bool followHead = true;
int saveSlot = 0;
bool showExitDialog = false;
int romSelection = 0;

float transitionPercentage = 1.0f;
int buttonState, lastButtonState;

std::string strForceDMG[] = {"Force DMG: Yes", "Force DMG: No"};
std::string strMove[] = {"Follow Head: Yes", "Follow Head: No"};

int button_mapping_menu;
int button_mapping_menu_index = 2;
std::string MapButtonStr[] = {"A", "B", "X", "Y"};
int MapButtons[] = {BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y};

const char *appDir;
const char *storageDir;

std::string saveFilePath;
std::string stateFolderPath;
std::string romFolderPath;

ovrVector4f sliderColor = {0.8f, 0.8f, 0.8f, 0.8f};
ovrVector4f textColorVersion = {0.8f, 0.8f, 0.8f, 0.8f};
ovrVector4f headerColor = {1.0f, 1.0f, 1.0f, 1.0f};

int strVersionWidth;
const ovrJava *java;

jclass clsData;
jmethodID getVal;

int batteryLevel, batter_string_width, time_string_width;
std::string time_string, battery_string;

int ButtonMappingIndex = 0;
bool UpdateMapping = false;
float MappingOverlayPercentage;

Vector4f MenuBackgroundColor{0.03f, 0.036f, 0.06f, 0.985f};
//Vector4f MenuBackgroundColor{0.0f, 0.0f, 0.0f, 0.0f};
Vector4f MenuBackgroundOverlayColor{0.0f, 0.0f, 0.0f, 0.25f};
Vector4f MenuBackgroundOverlayColorLight{0.0f, 0.0f, 0.0f, 0.15f};

Vector4f BatteryColor{1.0f, 1.0f, 1.0f, 1.0f};
Vector4f BatteryBackgroundColor{0.15f, 0.15f, 0.15f, 1.0f};

ovrTextureSwapChain *MenuSwapChain;
GLuint MenuFrameBuffer = 0;

FontManager::RenderFont fontHeader, fontBattery, fontTime, fontMenu, fontList, fontBottom, fontSlot,
    fontSmall;

GLuint textureBackgroundId, textureIdMenu, textureHeaderIconId, textureGbIconId, textureGbcIconId,
    textureSaveIconId,
    textureLoadIconId, textureWhiteId, textureResumeId, textureSettingsId, texuterLeftRightIconId,
    textureUpDownIconId, textureResetIconId, textureSaveSlotIconId, textureLoadRomIconId,
    textureBackIconId, textureMoveIconId, textureDistanceIconId, textureResetViewIconId,
    textureScaleIconId, textureMappingIconId, texturePaletteIconId, textureButtonAIconId,
    textureButtonBIconId, textureButtonXIconId, textureButtonYIconId, textureFollowHeadIconId,
    textureDMGIconId, textureExitIconId;

template<typename T>
std::string to_string(T value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

bool isTransitioning;
int transitionMoveDir = 1;
float transitionState = 1;
Menu *currentMenu, *nextMenu, *currentBottomBar;
Menu romSelectionMenu, mainMenu, settingsMenu, moveMenu, buttonMapMenu, bottomBar,
    buttonMappingOverlay;

int *remapButton;
MenuButton *mappedButton;
void (*updateMappingText)() = nullptr;
std::vector<int> possibleMappingIndices;

MenuButton *backHelp, *menuHelp, *selectHelp;
MenuList *romList;
MenuLabel *emptySlotLabel, *noImageSlotLabel;
MenuLabel *mappingButtonLabel, *possibleMappingLabel;

MenuButton *menuMappingButton;
MenuButton *buttonMapping[2];
MenuImage *imagePalette[4];

MenuButton *slotButton;
MenuButton *yawButton;
MenuButton *pitchButton;
MenuButton *rollButton;
MenuButton *scaleButton;
MenuButton *distanceButton;

#if defined(OVR_OS_ANDROID)

extern "C" {

jlong Java_com_a_gear_boy_go_MainActivity_nativeSetAppInterface(JNIEnv *jni, jclass clazz,
                                                                jobject activity,
                                                                jstring fromPackageName,
                                                                jstring commandString,
                                                                jstring uriString) {
  jmethodID messageMe = jni->GetMethodID(clazz, "getInternalStorageDir", "()Ljava/lang/String;");
  jobject result = (jstring) jni->CallObjectMethod(activity, messageMe);
  storageDir = jni->GetStringUTFChars((jstring) result, NULL);

  messageMe = jni->GetMethodID(clazz, "getExternalFilesDir", "()Ljava/lang/String;");
  result = (jstring) jni->CallObjectMethod(activity, messageMe);
  appDir = jni->GetStringUTFChars((jstring) result, NULL);

  saveFilePath = appDir;
  saveFilePath.append("/settings.config");

  stateFolderPath = storageDir;
  stateFolderPath += "/Roms/GB/States/";

  romFolderPath = storageDir;
  romFolderPath += "/Roms/GB/";

  LOG("got string from java: appdir %s", appDir);
  LOG("got string from java: storageDir %s", storageDir);

  LOG("nativeSetAppInterface");
  return (new OvrApp())
      ->SetActivity(jni, clazz, activity, fromPackageName, commandString, uriString);
}

}  // extern "C"

#endif

void MenuList::DrawTexture(float offsetX, float offsetY, float transparency) {
  // calculate the slider position
  float scale = maxListItems / (float) RomList->size();
  if (scale > 1) scale = 1;
  GLfloat recHeight = scrollbarHeight * scale;

  GLfloat sliderPercentage = 0;
  if (RomList->size() > maxListItems)
    sliderPercentage = (menuListState / (float) (RomList->size() - maxListItems));
  else
    sliderPercentage = 0;

  GLfloat recPosY = (scrollbarHeight - recHeight) * sliderPercentage;

  // slider background
  DrawHelper::DrawTexture(textureWhiteId, PosX + offsetX + 2, PosY + 2, scrollbarWidth - 4,
                          scrollbarHeight - 4, MenuBackgroundOverlayColor, transparency);
  // slider
  DrawHelper::DrawTexture(textureWhiteId, PosX + offsetX, PosY + recPosY, scrollbarWidth, recHeight,
                          sliderColor, transparency);

  // draw the cartridge icons
  for (uint i = (uint) menuListState; i < menuListState + maxListItems; i++) {
    if (i < RomList->size()) {
      DrawHelper::DrawTexture(
          RomList->at(i).isGbc ? textureGbcIconId : textureGbIconId,
          PosX + offsetX + scrollbarWidth + 15 + (((uint) CurrentSelection == i) ? 5 : 0),
          listStartY + listItemSize / 2 - 12 + listItemSize * (i - menuListState) + offsetY,
          21,
          24,
          {1.0f, 1.0f, 1.0f, 1.0f},
          transparency);
    }
  }
}

OvrApp::OvrApp()
    : SoundEffectContext(NULL),
      SoundEffectPlayer(NULL),
      GuiSys(OvrGuiSys::Create()),
      Locale(NULL),
      SceneModel(NULL) {}

OvrApp::~OvrApp() {
  delete SoundEffectPlayer;
  SoundEffectPlayer = NULL;

  delete SoundEffectContext;
  SoundEffectContext = NULL;

  OvrGuiSys::Destroy(GuiSys);
  if (SceneModel != NULL) {
    delete SceneModel;
  }
}

void OvrApp::Configure(ovrSettings &settings) {
  settings.CpuLevel = 0;
  settings.GpuLevel = 0;

  settings.RenderMode = RENDERMODE_MULTIVIEW;
  settings.UseSrgbFramebuffer = true;
}

void OvrApp::EnteredVrMode(const ovrIntentType intentType, const char *intentFromPackage,
                           const char *intentJSON, const char *intentURI) {
  OVR_UNUSED(intentFromPackage);
  OVR_UNUSED(intentJSON);
  OVR_UNUSED(intentURI);

  if (intentType == INTENT_LAUNCH) {
    FontManager::Init(menuWidth, menuHeight);
    FontManager::LoadFont(&fontHeader, "/system/fonts/Roboto-Regular.ttf", 55);
    FontManager::LoadFont(&fontBattery, "/system/fonts/Roboto-Bold.ttf", 20);
    fontTime = fontBattery;
    FontManager::LoadFont(&fontMenu, "/system/fonts/Roboto-Regular.ttf", 24);
    FontManager::LoadFont(&fontList, "/system/fonts/Roboto-Regular.ttf", 22);
    FontManager::LoadFont(&fontBottom, "/system/fonts/Roboto-Regular.ttf", 22);

    FontManager::LoadFont(&fontSmall, "/system/fonts/Roboto-Light.ttf", 14);
    FontManager::LoadFont(&fontSlot, "/system/fonts/Roboto-Regular.ttf", 26);
    FontManager::CloseFontLoader();

    textureBackgroundId = TextureLoader::Load(app, "apk:///assets/background2.dds");

    textureHeaderIconId = TextureLoader::Load(app, "apk:///assets/header_icon.dds");
    textureGbIconId = TextureLoader::Load(app, "apk:///assets/gb_cartridge.dds");
    textureGbcIconId = TextureLoader::Load(app, "apk:///assets/gbc_cartridge.dds");
    textureSaveIconId = TextureLoader::Load(app, "apk:///assets/save_icon.dds");
    textureLoadIconId = TextureLoader::Load(app, "apk:///assets/load_icon.dds");
    textureResumeId = TextureLoader::Load(app, "apk:///assets/resume_icon.dds");
    textureSettingsId = TextureLoader::Load(app, "apk:///assets/settings_icon.dds");
    texuterLeftRightIconId = TextureLoader::Load(app, "apk:///assets/leftright_icon.dds");
    textureUpDownIconId = TextureLoader::Load(app, "apk:///assets/updown_icon.dds");
    textureResetIconId = TextureLoader::Load(app, "apk:///assets/reset_icon.dds");
    textureSaveSlotIconId = TextureLoader::Load(app, "apk:///assets/save_slot_icon.dds");
    textureLoadRomIconId = TextureLoader::Load(app, "apk:///assets/rom_list_icon.dds");
    textureMoveIconId = TextureLoader::Load(app, "apk:///assets/move_icon.dds");
    textureBackIconId = TextureLoader::Load(app, "apk:///assets/back_icon.dds");
    textureDistanceIconId = TextureLoader::Load(app, "apk:///assets/distance_icon.dds");
    textureResetViewIconId = TextureLoader::Load(app, "apk:///assets/reset_view_icon.dds");
    textureScaleIconId = TextureLoader::Load(app, "apk:///assets/scale_icon.dds");
    textureMappingIconId = TextureLoader::Load(app, "apk:///assets/mapping_icon.dds");
    texturePaletteIconId = TextureLoader::Load(app, "apk:///assets/palette_icon.dds");
    textureButtonAIconId = TextureLoader::Load(app, "apk:///assets/button_a_icon.dds");
    textureButtonBIconId = TextureLoader::Load(app, "apk:///assets/button_b_icon.dds");
    textureButtonXIconId = TextureLoader::Load(app, "apk:///assets/button_x_icon.dds");
    textureButtonYIconId = TextureLoader::Load(app, "apk:///assets/button_y_icon.dds");
    textureFollowHeadIconId = TextureLoader::Load(app, "apk:///assets/follow_head_icon.dds");
    textureDMGIconId = TextureLoader::Load(app, "apk:///assets/force_dmg_icon.dds");
    textureExitIconId = TextureLoader::Load(app, "apk:///assets/exit_icon.dds");

    textureWhiteId = TextureLoader::CreateWhiteTexture();

    DrawHelper::Init(menuWidth, menuHeight);

    LoadSettings();

    Emulator::Init(stateFolderPath);

    LOG("Scan dir");
    ScanDirectory();

    SetUpMenu();

    CreateScreen();

    // used to get the battery level
    java = app->GetJava();
    clsData = java->Env->GetObjectClass(java->ActivityObject);
    getVal = java->Env->GetMethodID(clsData, "GetBatteryLevel", "()I");

  } else if (intentType == INTENT_NEW) {
  }
}

int UpdateBatteryLevel() {
  jint bLevel = java->Env->CallIntMethod(java->ActivityObject, getVal);
  int returnValue = (int) bLevel;
  return returnValue;
}

void CreateScreen() {
  // menu layer
  MenuSwapChain = vrapi_CreateTextureSwapChain(VRAPI_TEXTURE_TYPE_2D, VRAPI_TEXTURE_FORMAT_8888,
                                               menuWidth, menuHeight, 1, false);

  textureIdMenu = vrapi_GetTextureSwapChainHandle(MenuSwapChain, 0);
  glBindTexture(GL_TEXTURE_2D, textureIdMenu);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, menuWidth, menuHeight, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindTexture(GL_TEXTURE_2D, 0);

  // create hte framebuffer for the menu texture
  glGenFramebuffers(1, &MenuFrameBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, MenuFrameBuffer);

  // Set "renderedTexture" as our colour attachement #0
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         (GLuint) textureIdMenu,
                         0);

  // Set the list of draw buffers.
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  LOG("finished creating screens");
}

void SaveSettings() {
  std::ofstream saveFile(saveFilePath, std::ios::trunc | std::ios::binary);
  saveFile.write(reinterpret_cast<const char *>(&SAVE_FILE_VERSION), sizeof(int));

  Emulator::SaveSettings(&saveFile);
  saveFile.write(reinterpret_cast<const char *>(&romList->CurrentSelection), sizeof(int));
  saveFile.write(reinterpret_cast<const char *>(&LayerBuilder::screenPitch), sizeof(float));
  saveFile.write(reinterpret_cast<const char *>(&LayerBuilder::screenYaw), sizeof(float));
  saveFile.write(reinterpret_cast<const char *>(&LayerBuilder::screenRoll), sizeof(float));
  saveFile.write(reinterpret_cast<const char *>(&LayerBuilder::radiusMenuScreen), sizeof(float));
  saveFile.write(reinterpret_cast<const char *>(&LayerBuilder::screenSize), sizeof(float));
  saveFile.write(reinterpret_cast<const char *>(&followHead), sizeof(bool));
  saveFile.write(reinterpret_cast<const char *>(&button_mapping_menu_index), sizeof(int));
  saveFile.write(reinterpret_cast<const char *>(&SwappSelectBackButton), sizeof(bool));

  saveFile.close();
  LOG("Saved Settings");
}

void LoadSettings() {
  std::ifstream loadFile(saveFilePath, std::ios::in | std::ios::binary | std::ios::ate);
  if (loadFile.is_open()) {
    loadFile.seekg(0, std::ios::beg);

    int saveFileVersion = 0;
    loadFile.read((char *) &saveFileVersion, sizeof(int));

    // only load if the save versions are compatible
    if (saveFileVersion == SAVE_FILE_VERSION) {
      Emulator::LoadSettings(&loadFile);
      loadFile.read((char *) &romSelection, sizeof(int));
      loadFile.read((char *) &LayerBuilder::screenPitch, sizeof(float));
      loadFile.read((char *) &LayerBuilder::screenYaw, sizeof(float));
      loadFile.read((char *) &LayerBuilder::screenRoll, sizeof(float));
      loadFile.read((char *) &LayerBuilder::radiusMenuScreen, sizeof(float));
      loadFile.read((char *) &LayerBuilder::screenSize, sizeof(float));
      loadFile.read((char *) &followHead, sizeof(bool));
      loadFile.read((char *) &button_mapping_menu_index, sizeof(int));
      loadFile.read((char *) &SwappSelectBackButton, sizeof(bool));
    }

    // TODO: reset all loaded settings
    if (loadFile.fail())
      LOG("Failed Loading Settings");
    else
      LOG("Settings Loaded");

    loadFile.close();
  }

  LOG("menu index %i", romSelection);
  button_mapping_menu = MapButtons[button_mapping_menu_index];
}

void ScanDirectory() {
  DIR *dir;
  struct dirent *ent;
  std::string fullPath;

  if ((dir = opendir(romFolderPath.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      fullPath = "";
      fullPath.append(romFolderPath);
      fullPath.append(ent->d_name);

      // check if it is a file
      // gb|dmg|gbc|cgb|sgb
      if (ent->d_type == 8) {
        std::string strFilename = ent->d_name;

        if (strFilename.find(".gb") != std::string::npos ||
            strFilename.find(".dmg") != std::string::npos ||
            strFilename.find(".gbc") != std::string::npos ||
            strFilename.find(".cgb") != std::string::npos ||
            strFilename.find(".sgb") != std::string::npos) {
          size_t lastIndex = strFilename.find_last_of(".");
          std::string listName = strFilename.substr(0, lastIndex);
          size_t lastIndexSave = (fullPath).find_last_of(".");
          std::string listNameSave = fullPath.substr(0, lastIndexSave);

          Emulator::Rom newRom;
          newRom.RomName = listName;
          newRom.FullPath = fullPath;
          newRom.FullPathNorm = listNameSave;
          newRom.SavePath = listNameSave + ".srm";

          // check if it is a gbc rom
          newRom.isGbc = (strFilename.find(".gbc") != std::string::npos ||
              strFilename.find(".cgb") != std::string::npos);

          romFileList->push_back(newRom);

          LOG("found rom: %s %s %s", newRom.RomName.c_str(), newRom.FullPath.c_str(),
              newRom.SavePath.c_str());
        }
      }
    }
    closedir(dir);
    LOG("sort list");
    std::sort(romFileList->begin(), romFileList->end(), Emulator::SortByRomName);
    LOG("finished sorting list");
  } else {
    LOG("could not open folder");
  }
}

void OvrApp::LeavingVrMode() {}

void GetTimeString(std::string &timeString) {
  struct timespec res;
  clock_gettime(CLOCK_REALTIME, &res);
  time_t t = res.tv_sec;  // just in case types aren't the same
  tm tmv;
  localtime_r(&t, &tmv);  // populate tmv with local time info

  timeString.clear();
  if (tmv.tm_hour < 10) timeString.append("0");
  timeString.append(to_string(tmv.tm_hour));
  timeString.append(":");
  if (tmv.tm_min < 10) timeString.append("0");
  timeString.append(to_string(tmv.tm_min));

  time_string_width = FontManager::GetWidth(fontTime, timeString);
}

void GetBattryString(std::string &batteryString) {
  batteryLevel = UpdateBatteryLevel();
  batteryString.clear();
  batteryString.append(to_string(batteryLevel));
  batteryString.append("%");

  batter_string_width = FontManager::GetWidth(fontBattery, batteryString);
}

void UpdateMenu() {
  if (isTransitioning) {
    transitionState -= MENU_TRANSITION_SPEED;
    if (transitionState < 0) {
      transitionState = 1;
      isTransitioning = false;
      currentMenu = nextMenu;
    }
  } else {
    // @hack: this should be done nicer
    emptySlotLabel->Visible = !Emulator::currentGame->saveStates[saveSlot].hasState;
    noImageSlotLabel->Visible = Emulator::currentGame->saveStates[saveSlot].hasState &&
        !Emulator::currentGame->saveStates[saveSlot].hasImage;

    currentMenu->Update(buttonState, lastButtonState);
  }
}

void DrawMenu() {
  // the
  float trProgressOut = ((transitionState - 0.35f) / 0.65f);
  float progressOut = sinf(trProgressOut * MATH_FLOAT_PIOVER2);
  if (trProgressOut < 0)
    trProgressOut = 0;

  float trProgressIn = (((1 - transitionState) - 0.35f) / 0.65f);
  float progressIn = sinf(trProgressIn * MATH_FLOAT_PIOVER2);
  if (transitionState < 0)
    trProgressIn = 0;

  int dist = 75;

  if (UpdateMapping) {
    MappingOverlayPercentage += 0.2f;
    if (MappingOverlayPercentage > 1)
      MappingOverlayPercentage = 1;
  } else {
    MappingOverlayPercentage -= 0.2f;
    if (MappingOverlayPercentage < 0)
      MappingOverlayPercentage = 0;
  }

  // draw the current menu
  currentMenu->Draw(-transitionMoveDir, 0, (1 - progressOut), dist, trProgressOut);
  // draw the next menu fading in
  if (isTransitioning)
    nextMenu->Draw(transitionMoveDir, 0, (1 - progressIn), dist, trProgressIn);

  // draw the bottom bar
  currentBottomBar->Draw(0, 0, 0, 0, 1);

  if (MappingOverlayPercentage) {
    buttonMappingOverlay.Draw(0,
                              -1,
                              (1 - sinf(MappingOverlayPercentage * MATH_FLOAT_PIOVER2)),
                              dist,
                              MappingOverlayPercentage);
  }

  /*
  DrawHelper::DrawTexture(textureWhiteId,
                          0,
                          200,
                          fontMenu.FontSize * 30,
                          fontMenu.FontSize * 8,
                          {0.0f, 0.0f, 0.0f, 1.0f},
                          1.0f);

  FontManager::Begin();
  FontManager::RenderFontImage(fontMenu, {1.0f, 1.0f, 1.0f, 1.0f}, 1.0f);
  FontManager::Close(); */

}

void DrawGUI() {
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  glBindFramebuffer(GL_FRAMEBUFFER, MenuFrameBuffer);
  // Render on the whole framebuffer, complete from the lower left corner to the
  // upper right
  glViewport(0, 0, menuWidth, menuHeight);

  glClearColor(MenuBackgroundColor.x, MenuBackgroundColor.y, MenuBackgroundColor.z,
               MenuBackgroundColor.w);
  glClear(GL_COLOR_BUFFER_BIT);

  // draw the backgroud image
  //DrawHelper::DrawTexture(textureBackgroundId, 0, 0, menuWidth, menuHeight,
  //                        {0.7f,0.7f,0.7f,1.0f}, 0.985f);

  // header
  DrawHelper::DrawTexture(textureWhiteId, 0, 0, menuWidth, HEADER_HEIGHT,
                          MenuBackgroundOverlayColor, 1);
  DrawHelper::DrawTexture(textureWhiteId, 0, menuHeight - BOTTOM_HEIGHT, menuWidth, BOTTOM_HEIGHT,
                          MenuBackgroundOverlayColorLight, 1);

  // icon
  DrawHelper::DrawTexture(textureHeaderIconId, 0, 0, 75, 75, headerColor, 1);

  FontManager::Begin();
  FontManager::RenderText(fontHeader,
                          STR_HEADER,
                          75,
                          HEADER_HEIGHT / 2 - fontHeader.PHeight / 2 - fontHeader.PStart,
                          1.0f,
                          headerColor,
                          1);

  // update the battery string
  int batteryWidth = 10;
  int maxHeight = fontBattery.PHeight + 2;
  int batteryOffset = fontBattery.PStart - 1;
  int distX = 22;
  int distY = 16;
  GetBattryString(battery_string);
  FontManager::RenderText(fontBattery, battery_string,
                          menuWidth - batter_string_width - batteryWidth - 5 - distX, distY, 1.0f,
                          textColorVersion, 1);

  // update the time string
  GetTimeString(time_string);
  FontManager::RenderText(fontTime, time_string, menuWidth - time_string_width - distX,
                          HEADER_HEIGHT - distY - fontTime.FontSize, 1.0f, textColorVersion, 1);

  // FontManager::RenderText(fontSmall, STR_VERSION, menuWidth - strVersionWidth - 7.0f,
  //                        HEADER_HEIGHT - 21, 1.0f, textColorVersion, 1);
  FontManager::Close();

  // draw battery
  DrawHelper::DrawTexture(textureWhiteId,
                          menuWidth - batteryWidth - distX,
                          distY + batteryOffset,
                          batteryWidth,
                          maxHeight,
                          BatteryBackgroundColor,
                          1);
  int height = (int) (batteryLevel / 100.0 * maxHeight);
  DrawHelper::DrawTexture(textureWhiteId,
                          menuWidth - batteryWidth - distX,
                          distY + batteryOffset + maxHeight - height,
                          batteryWidth,
                          height,
                          BatteryColor,
                          1);

  DrawMenu();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ovrFrameResult OvrApp::Frame(const ovrFrameInput &vrFrame) {
  if (showExitDialog) {
    OvrApp::app->ShowSystemUI(VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
    showExitDialog = false;
  }

  buttonState = vrFrame.Input.buttonState;

  if (!menuOpen) {
    if (transitionPercentage > 0) transitionPercentage -= OPEN_MENU_SPEED;
    if (transitionPercentage < 0) transitionPercentage = 0;

    Emulator::Update(vrFrame, lastButtonState);
  } else {
    if (transitionPercentage < 1) transitionPercentage += OPEN_MENU_SPEED;
    if (transitionPercentage > 1) transitionPercentage = 1;

    UpdateMenu();
  }

  // open/close menu
  if (buttonState & button_mapping_menu && !(lastButtonState & button_mapping_menu) &&
      loadedRom) {
    menuOpen = !menuOpen;
  }

  ovrFrameResult res;

  // Clear the eye buffers to 0 alpha so the overlay plane shows through.
  res.ClearColor = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
  res.ClearColorBuffer = true;

  res.FrameIndex = vrFrame.FrameNumber;
  res.DisplayTime = vrFrame.PredictedDisplayTimeInSeconds;
  res.SwapInterval = app->GetSwapInterval();

  res.FrameFlags = 0;
  res.LayerCount = 0;

  LayerBuilder::UpdateDirection(vrFrame);

  ovrLayerProjection2 &worldLayer = res.Layers[res.LayerCount++].Projection;
  worldLayer = vrapi_DefaultLayerProjection2();
  worldLayer.HeadPose = vrFrame.Tracking.HeadPose;
  for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++) {
    res.FrameMatrices.EyeView[eye] = vrFrame.Tracking.Eye[eye].ViewMatrix;
    // Calculate projection matrix using custom near plane value.
    res.FrameMatrices.EyeProjection[eye] =
        ovrMatrix4f_CreateProjectionFov(vrFrame.FovX, vrFrame.FovY, 0.0f, 0.0f, 1.0f, 0.0f);

    worldLayer.Textures[eye].ColorSwapChain = vrFrame.ColorTextureSwapChain[eye];
    worldLayer.Textures[eye].SwapChainIndex = vrFrame.TextureSwapChainIndex;
    worldLayer.Textures[eye].TexCoordsFromTanAngles = vrFrame.TexCoordsFromTanAngles;
  }

  // virtual screen layer
  res.Layers[res.LayerCount].Cylinder =
      LayerBuilder::BuildGameCylinderLayer(Emulator::CylinderSwapChain, Emulator::CylinderWidth,
                                           Emulator::CylinderHeight, &vrFrame.Tracking, followHead);

  res.Layers[res.LayerCount].Cylinder.Header.Flags |=
      VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
  //res.Layers[res.LayerCount].Cylinder.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_ONE;
  //res.Layers[res.LayerCount].Cylinder.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ZERO;
  res.LayerCount++;

  if (transitionPercentage > 0) {
    // menu layer
    if (menuOpen) DrawGUI();

    float transitionP = sinf((transitionPercentage) * MATH_FLOAT_PIOVER2);

    res.Layers[res.LayerCount].Cylinder = LayerBuilder::BuildSettingsCylinderLayer(
        MenuSwapChain, menuWidth, menuHeight, &vrFrame.Tracking, followHead, transitionP);

    res.Layers[res.LayerCount].Cylinder.Header.Flags |=
        VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
    res.Layers[res.LayerCount].Cylinder.Header.ColorScale = {transitionP, transitionP, transitionP,
                                                             transitionP};
    res.Layers[res.LayerCount].Cylinder.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
    res.Layers[res.LayerCount].Cylinder.Header.DstBlend =
        VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
    res.LayerCount++;
  }

  lastButtonState = vrFrame.Input.buttonState;

  return res;
}

void StartTransition(Menu *next, int dir) {
  if (isTransitioning) return;
  isTransitioning = true;
  transitionMoveDir = dir;
  nextMenu = next;
}

void OnClickResumGame(MenuItem *item) {
  LOG("Pressed RESUME GAME");
  if (loadedRom) menuOpen = false;
}

void OnClickResetGame(MenuItem *item) {
  LOG("RESET GAME");
  if (loadedRom) {
    Emulator::ResetGame();
    menuOpen = false;
  }
}

void OnClickSaveGame(MenuItem *item) {
  LOG("on click save game");
  if (loadedRom) {
    Emulator::SaveState(saveSlot);
    menuOpen = false;
  }
}

void OnClickLoadGame(MenuItem *item) {
  if (loadedRom) {
    Emulator::LoadState(saveSlot);
    menuOpen = false;
  }
}

void OnClickLoadRomGame(MenuItem *item) { StartTransition(&romSelectionMenu, -1); }

void OnClickSettingsGame(MenuItem *item) { StartTransition(&settingsMenu, 1); }

void UpdatePalettes() {
  for (int i = 0; i < 4; ++i)
    imagePalette[i]->Color = {Emulator::palettes[Emulator::selectedPalette][i].red / 255.0f,
                              Emulator::palettes[Emulator::selectedPalette][i].green / 255.0f,
                              Emulator::palettes[Emulator::selectedPalette][i].blue / 255.0f,
                              Emulator::palettes[Emulator::selectedPalette][i].alpha / 255.0f};
}

void ChangePalette(MenuItem *item, int dir) {
  Emulator::ChangePalette(dir);
  UpdatePalettes();
  ((MenuButton *) item)->Text = "Palette: " + to_string(Emulator::selectedPalette + 1);
}

void OnClickChangePaletteLeft(MenuItem *item) {
  ChangePalette(item, -1);
  SaveSettings();
}

void OnClickChangePaletteRight(MenuItem *item) {
  ChangePalette(item, 1);
  SaveSettings();
}

void SetForceDMG(MenuItem *item, bool newForceDMG) {
  Emulator::forceDMG = newForceDMG;
  ((MenuButton *) item)->Text = strForceDMG[Emulator::forceDMG ? 0 : 1];
}

void OnClickEmulatedModel(MenuItem *item) {
  SetForceDMG(item, !Emulator::forceDMG);
  SaveSettings();
}

void OnClickFollowMode(MenuItem *item) {
  followHead = !followHead;
  ((MenuButton *) item)->Text = strMove[followHead ? 0 : 1];
  SaveSettings();
}

void OnClickMoveScreen(MenuItem *item) { StartTransition(&moveMenu, 1); }

void OnClickMappingScreen(MenuItem *item) { StartTransition(&buttonMapMenu, 1); }

void OnClickBackAndSave(MenuItem *item) {
  StartTransition(&mainMenu, -1);
  SaveSettings();
}

void OnBackPressedRomList() { StartTransition(&mainMenu, 1); }

void OnBackPressedSettings() {
  StartTransition(&mainMenu, -1);
  SaveSettings();
}

void OnClickBackMove(MenuItem *item) {
  StartTransition(&settingsMenu, -1);
  SaveSettings();
}

void ChangeSaveSlot(MenuItem *item, int dir) {
  saveSlot += dir;
  if (saveSlot < 0) saveSlot = MAX_SAVE_SLOTS - 1;
  if (saveSlot >= MAX_SAVE_SLOTS) saveSlot = 0;
  Emulator::UpdateStateImage(saveSlot);
  ((MenuButton *) item)->Text = "Save Slot: " + to_string(saveSlot);
}

void OnClickSaveSlotLeft(MenuItem *item) {
  ChangeSaveSlot(item, -1);
  SaveSettings();
}

void OnClickSaveSlotRight(MenuItem *item) {
  ChangeSaveSlot(item, 1);
  SaveSettings();
}

float ToDegree(float radian) { return (int) (180.0 / VRAPI_PI * radian * 10) / 10.0f; }

void MoveYaw(MenuItem *item, float dir) {
  LayerBuilder::screenYaw -= dir;
  ((MenuButton *) item)->Text = "Yaw: " + to_string(ToDegree(LayerBuilder::screenYaw)) + "°";
}

void MovePitch(MenuItem *item, float dir) {
  LayerBuilder::screenPitch -= dir;
  ((MenuButton *) item)->Text = "Pitch: " + to_string(ToDegree(LayerBuilder::screenPitch)) + "°";
}

void MoveRoll(MenuItem *item, float dir) {
  LayerBuilder::screenRoll -= dir;
  ((MenuButton *) item)->Text = "Roll: " + to_string(ToDegree(LayerBuilder::screenRoll)) + "°";
}

void ChangeDistance(MenuItem *item, float dir) {
  LayerBuilder::radiusMenuScreen -= dir;

  if (LayerBuilder::radiusMenuScreen < MIN_DISTANCE) LayerBuilder::radiusMenuScreen = MIN_DISTANCE;
  if (LayerBuilder::radiusMenuScreen > MAX_DISTANCE) LayerBuilder::radiusMenuScreen = MAX_DISTANCE;

  ((MenuButton *) item)->Text = "Distance: " + to_string(LayerBuilder::radiusMenuScreen);
}

void ChangeScale(MenuItem *item, float dir) {
  LayerBuilder::screenSize -= dir;

  if (LayerBuilder::screenSize < 0.5f) LayerBuilder::screenSize = 0.5f;
  if (LayerBuilder::screenSize > 2.0f) LayerBuilder::screenSize = 2.0f;

  ((MenuButton *) item)->Text = "Scale: " + to_string(LayerBuilder::screenSize);
}

void OnClickResetView(MenuItem *item) {
  LayerBuilder::ResetValues();

  // updates the visible values
  MoveYaw(yawButton, 0);
  MovePitch(pitchButton, 0);
  MoveRoll(rollButton, 0);
  ChangeDistance(distanceButton, 0);
  ChangeScale(scaleButton, 0);

  SaveSettings();
}

void OnClickYaw(MenuItem *item) {
  LayerBuilder::screenYaw = 0;
  MoveYaw(yawButton, 0);
  SaveSettings();
}

void OnClickPitch(MenuItem *item) {
  LayerBuilder::screenPitch = 0;
  MovePitch(pitchButton, 0);
  SaveSettings();
}

void OnClickRoll(MenuItem *item) {
  LayerBuilder::screenRoll = 0;
  MoveRoll(rollButton, 0);
  SaveSettings();
}

void OnClickDistance(MenuItem *item) {
  LayerBuilder::radiusMenuScreen = 0.75f;
  ChangeDistance(distanceButton, 0);
  SaveSettings();
}

void OnClickScale(MenuItem *item) {
  LayerBuilder::screenSize = 1.0f;
  ChangeScale(scaleButton, 0);
  SaveSettings();
}

void OnClickMoveScreenYawLeft(MenuItem *item) { MoveYaw(item, MoveSpeed); }

void OnClickMoveScreenYawRight(MenuItem *item) { MoveYaw(item, -MoveSpeed); }

void OnClickMoveScreenPitchLeft(MenuItem *item) { MovePitch(item, -MoveSpeed); }

void OnClickMoveScreenPitchRight(MenuItem *item) { MovePitch(item, MoveSpeed); }

void OnClickMoveScreenRollLeft(MenuItem *item) { MoveRoll(item, -MoveSpeed); }

void OnClickMoveScreenRollRight(MenuItem *item) { MoveRoll(item, MoveSpeed); }

void OnClickMoveScreenDistanceLeft(MenuItem *item) { ChangeDistance(item, ZoomSpeed); }

void OnClickMoveScreenDistanceRight(MenuItem *item) { ChangeDistance(item, -ZoomSpeed); }

void OnClickMoveScreenScaleLeft(MenuItem *item) { ChangeScale(item, MoveSpeed); }

void OnClickMoveScreenScaleRight(MenuItem *item) { ChangeScale(item, -MoveSpeed); }

void SwapButtonSelectBack(MenuItem *item) {
  SwappSelectBackButton = !SwappSelectBackButton;
  ((MenuButton *) item)->Text = "Swap Select and Back: ";
  ((MenuButton *) item)->Text.append((SwappSelectBackButton ? "Yes" : "No"));

  SelectButton = SwappSelectBackButton ? BUTTON_B : BUTTON_A;
  BackButton = SwappSelectBackButton ? BUTTON_A : BUTTON_B;

  selectHelp->IconId = SwappSelectBackButton ? textureButtonBIconId : textureButtonAIconId;
  backHelp->IconId = SwappSelectBackButton ? textureButtonAIconId : textureButtonBIconId;
}

int GetPressedButton(int &_buttonState, int &_lastButtonState) {
  if (_buttonState & BUTTON_A && !(_lastButtonState & BUTTON_A)) {
    return 0;
  }
  if (_buttonState & BUTTON_B && !(_lastButtonState & BUTTON_B)) {
    return 1;
  }
  if (_buttonState & BUTTON_X && !(_lastButtonState & BUTTON_X)) {
    return 2;
  }
  if (_buttonState & BUTTON_Y && !(_lastButtonState & BUTTON_Y)) {
    return 3;
  }

  return -1;
}

void MoveMenuButtonMapping(MenuItem *item, int dir) {
  button_mapping_menu_index += dir;
  if (button_mapping_menu_index > 3) button_mapping_menu_index = 2;

  button_mapping_menu = MapButtons[button_mapping_menu_index];
  ((MenuButton *) item)->Text = "menu mapped to: " + MapButtonStr[button_mapping_menu_index];

  menuHelp->IconId = button_mapping_menu_index == 2 ? textureButtonXIconId : textureButtonYIconId;
}

// mapping functions
void MoveAButtonMapping(MenuItem *item, int dir) {
  Emulator::ChangeButtonMapping(0, dir);
  ((MenuButton *) item)->Text = "mapped to: " + MapButtonStr[Emulator::button_mapping_index[0]];
}

void MoveBButtonMapping(MenuItem *item, int dir) {
  Emulator::ChangeButtonMapping(1, dir);
  ((MenuButton *) item)->Text = "mapped to: " + MapButtonStr[Emulator::button_mapping_index[1]];
}

void UpdateMenuMapping() {
  button_mapping_menu = MapButtons[button_mapping_menu_index];
  mappedButton->Text = "menu mapped to: " + MapButtonStr[button_mapping_menu_index];
}

void UpdateEmulatorMapping() {
  Emulator::UpdateButtonMapping();
  mappedButton->Text = "mapped to: " + MapButtonStr[*remapButton];
}

void OnClickChangeMenuButtonLeft(MenuItem *item) { MoveMenuButtonMapping(item, 1); }
void OnClickChangeMenuButtonRight(MenuItem *item) { MoveMenuButtonMapping(item, 1); }
void OnClickChangeMenuButtonEnter(MenuItem *item) {
  UpdateMapping = true;
  remapButton = &button_mapping_menu_index;
  mappedButton = menuMappingButton;
  updateMappingText = UpdateMenuMapping;

  possibleMappingIndices.clear();
  possibleMappingIndices.push_back(2);
  possibleMappingIndices.push_back(3);

  mappingButtonLabel->SetText("Menu Button");
  possibleMappingLabel->SetText("(X, Y)");
}

void OnClickChangeAButtonLeft(MenuItem *item) { MoveAButtonMapping(item, -1); }
void OnClickChangeAButtonRight(MenuItem *item) { MoveAButtonMapping(item, 1); }
void OnClickChangeAButtonEnter(MenuItem *item) {
  UpdateMapping = true;
  remapButton = &Emulator::button_mapping_index[0];
  mappedButton = buttonMapping[0];
  updateMappingText = UpdateEmulatorMapping;

  possibleMappingIndices.clear();
  possibleMappingIndices.push_back(0);
  possibleMappingIndices.push_back(1);
  possibleMappingIndices.push_back(2);
  possibleMappingIndices.push_back(3);

  mappingButtonLabel->SetText("A Button");
  possibleMappingLabel->SetText("(A, B, X, Y)");
}

void OnClickChangeBButtonLeft(MenuItem *item) { MoveBButtonMapping(item, -1); }
void OnClickChangeBButtonRight(MenuItem *item) { MoveBButtonMapping(item, 1); }
void OnClickChangeBButtonEnter(MenuItem *item) {
  UpdateMapping = true;
  remapButton = &Emulator::button_mapping_index[1];
  mappedButton = buttonMapping[1];
  updateMappingText = UpdateEmulatorMapping;

  possibleMappingIndices.clear();
  possibleMappingIndices.push_back(0);
  possibleMappingIndices.push_back(1);
  possibleMappingIndices.push_back(2);
  possibleMappingIndices.push_back(3);

  mappingButtonLabel->SetText("B Button");
  possibleMappingLabel->SetText("(A, B, X, Y)");
}

void UpdateButtonMapping(MenuItem *item, int &_buttonState, int &_lastButtonState) {
  if (UpdateMapping) {
    int newButtonState = GetPressedButton(_buttonState, _lastButtonState);

    for (int i = 0; i < possibleMappingIndices.size(); ++i) {
      if (newButtonState == possibleMappingIndices.at(i)) {
        UpdateMapping = false;
        *remapButton = newButtonState;
        updateMappingText();
      }
    }

    _buttonState = 0;
    _lastButtonState = 0;
  }
}

void OnBackPressedMove() {
  StartTransition(&settingsMenu, -1);
  SaveSettings();
}

void OnClickExit(MenuItem *item) {
  Emulator::SaveRam();
  showExitDialog = true;
}

void OnClickRom(Emulator::Rom *rom) {
  LOG("LOAD ROM");
  SaveSettings();
  saveSlot = 0;
  ChangeSaveSlot(slotButton, 0);
  Emulator::LoadGame(rom);
  currentMenu = &mainMenu;
  menuOpen = false;
  loadedRom = true;
}

void OnClickBackMainMenu() {
  if (loadedRom) menuOpen = false;
}

void OvrApp::SetUpMenu() {
  int bigGap = 10;
  int smallGap = 5;

  menuItemSize = (fontMenu.FontSize + 4);
  strVersionWidth = GetWidth(fontSmall, STR_VERSION);

  romSelectionMenu.CurrentSelection = 0;
  romList = new MenuList(&fontList, OnClickRom, romFileList, 10, HEADER_HEIGHT + 10, menuWidth - 20,
                         (menuHeight - HEADER_HEIGHT - BOTTOM_HEIGHT - 20));
  romList->CurrentSelection = romSelection;
  romSelectionMenu.MenuItems.push_back(romList);
  romSelectionMenu.BackPress = OnBackPressedRomList;
  romSelectionMenu.Init();

  menuHelp = new MenuButton(&fontBottom,
                            button_mapping_menu_index == 2 ? textureButtonXIconId
                                                           : textureButtonYIconId,
                            "Close Menu",
                            7,
                            menuHeight - BOTTOM_HEIGHT,
                            BOTTOM_HEIGHT,
                            nullptr,
                            nullptr,
                            nullptr);
  backHelp = new MenuButton(&fontBottom,
                            SwappSelectBackButton ? textureButtonAIconId : textureButtonBIconId,
                            "Back",
                            menuWidth - 210,
                            menuHeight - BOTTOM_HEIGHT,
                            BOTTOM_HEIGHT,
                            nullptr,
                            nullptr,
                            nullptr);
  selectHelp = new MenuButton(&fontBottom,
                              SwappSelectBackButton ? textureButtonBIconId : textureButtonAIconId,
                              "Select",
                              menuWidth - 110,
                              menuHeight - BOTTOM_HEIGHT,
                              BOTTOM_HEIGHT,
                              nullptr,
                              nullptr,
                              nullptr);

  bottomBar.MenuItems.push_back(backHelp);
  bottomBar.MenuItems.push_back(selectHelp);
  bottomBar.MenuItems.push_back(menuHelp);
  currentBottomBar = &bottomBar;

  // main menu page
  int posX = 20;
  int posY = HEADER_HEIGHT + 20;
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureResumeId, "Resume Game", posX, posY,
                                              OnClickResumGame, nullptr, nullptr));
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureResetIconId, "Reset Game", posX,
                                              posY += menuItemSize, OnClickResetGame, nullptr,
                                              nullptr));
  slotButton = new MenuButton(&fontMenu, textureSaveSlotIconId, "", posX, posY += menuItemSize + 10,
                              OnClickSaveSlotRight, OnClickSaveSlotLeft, OnClickSaveSlotRight);
  ChangeSaveSlot(slotButton, 0);
  mainMenu.MenuItems.push_back(slotButton);
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                              textureSaveIconId,
                                              "Save",
                                              posX,
                                              posY += menuItemSize,
                                              OnClickSaveGame,
                                              nullptr,
                                              nullptr));
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                              textureLoadIconId,
                                              "Load",
                                              posX,
                                              posY += menuItemSize,
                                              OnClickLoadGame,
                                              nullptr,
                                              nullptr));
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureLoadRomIconId, "Load Rom", posX,
                                              posY += menuItemSize + 10, OnClickLoadRomGame,
                                              nullptr, nullptr));
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureSettingsId, "Settings", posX,
                                              posY += menuItemSize, OnClickSettingsGame, nullptr,
                                              nullptr));
  mainMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                              textureExitIconId,
                                              "Exit",
                                              posX,
                                              posY += menuItemSize + 10,
                                              OnClickExit,
                                              nullptr,
                                              nullptr));

  // background
  mainMenu.MenuItems.push_back(new MenuImage(textureWhiteId, menuWidth - 320 - 20 - 5,
                                             HEADER_HEIGHT + 20 - 5, 320 + 10, 288 + 10,
                                             MenuBackgroundOverlayColor));
  emptySlotLabel = new MenuLabel(&fontSlot, "- Empty Slot -", menuWidth - 320 - 20,
                                 HEADER_HEIGHT + 20, 320, 288, {1.0f, 1.0f, 1.0f, 1.0f});
  noImageSlotLabel = new MenuLabel(&fontSlot, "- -", menuWidth - 320 - 20,
                                   HEADER_HEIGHT + 20, 320, 288, {1.0f, 1.0f, 1.0f, 1.0f});
  mainMenu.MenuItems.push_back(emptySlotLabel);
  mainMenu.MenuItems.push_back(noImageSlotLabel);

  // image slot
  mainMenu.MenuItems.push_back(new MenuImage(Emulator::stateImageId, menuWidth - 320 - 20,
                                             HEADER_HEIGHT + 20, 320, 288,
                                             {1.0f, 1.0f, 1.0f, 1.0f}));

  mainMenu.Init();
  mainMenu.BackPress = OnClickBackMainMenu;

  // settings page
  posY = HEADER_HEIGHT + 20;

  settingsMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                                  textureMappingIconId,
                                                  "Button Mapping",
                                                  posX,
                                                  posY,
                                                  OnClickMappingScreen,
                                                  nullptr,
                                                  nullptr));
  settingsMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureMoveIconId, "Move Screen", posX,
                                                  posY += menuItemSize, OnClickMoveScreen, nullptr,
                                                  nullptr));
  settingsMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                                  textureFollowHeadIconId,
                                                  strMove[followHead ? 0 : 1],
                                                  posX,
                                                  posY += menuItemSize + bigGap,
                                                  OnClickFollowMode,
                                                  OnClickFollowMode,
                                                  OnClickFollowMode));

  MenuContainer *menuContainer = new MenuContainer();
  MenuButton *paletteButton =
      new MenuButton(&fontMenu, texturePaletteIconId, "", posX, posY += menuItemSize,
                     OnClickChangePaletteRight, OnClickChangePaletteLeft,
                     OnClickChangePaletteRight);
  menuContainer->MenuItems.push_back(paletteButton);
  for (int i = 0; i < 4; ++i) {
    imagePalette[i] = new MenuImage(textureWhiteId,
                                    posX + (18) * i + 157,
                                    posY + fontMenu.FontSize / 2 - 9,
                                    18,
                                    18,
                                    {0.0f, 0.0f, 0.0f, 0.0f});
    menuContainer->MenuItems.push_back(imagePalette[i]);
  }
  UpdatePalettes();

  MenuButton *dmgButton =
      new MenuButton(&fontMenu,
                     textureDMGIconId,
                     "",
                     posX,
                     posY += menuItemSize,
                     OnClickEmulatedModel,
                     OnClickEmulatedModel,
                     OnClickEmulatedModel);

  settingsMenu.MenuItems.push_back(menuContainer);

  settingsMenu.MenuItems.push_back(dmgButton);
  settingsMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureBackIconId, "Back", posX,
                                                  posY += menuItemSize + bigGap, OnClickBackAndSave,
                                                  nullptr, nullptr));

  // state screenshot background
  settingsMenu.MenuItems.push_back(new MenuImage(textureWhiteId, menuWidth - 320 - 20 - 5,
                                                 HEADER_HEIGHT + 20 - 5, 320 + 10, 288 + 10,
                                                 MenuBackgroundOverlayColor));
  settingsMenu.MenuItems.push_back(new MenuImage(Emulator::screenTextureId, menuWidth - 320 - 20,
                                                 HEADER_HEIGHT + 20, 320, 288,
                                                 {1.0f, 1.0f, 1.0f, 1.0f}));

  settingsMenu.BackPress = OnBackPressedSettings;
  settingsMenu.Init();

  // set text
  ChangePalette(paletteButton, 0);
  SetForceDMG(dmgButton, Emulator::forceDMG);

  // button mapping page
  posY = HEADER_HEIGHT + 20;

  MenuButton *swapButton =
      new MenuButton(&fontMenu, textureLoadRomIconId, "", posX, posY, SwapButtonSelectBack,
                     SwapButtonSelectBack, SwapButtonSelectBack);
  menuMappingButton =
      new MenuButton(&fontMenu,
                     textureLoadRomIconId,
                     "",
                     posX,
                     posY += menuItemSize,
                     OnClickChangeMenuButtonEnter,
                     OnClickChangeMenuButtonLeft,
                     OnClickChangeMenuButtonRight);

  buttonMapping[0] =
      new MenuButton(&fontMenu, textureButtonAIconId, "", posX, posY += menuItemSize + smallGap,
                     OnClickChangeAButtonEnter, OnClickChangeAButtonLeft,
                     OnClickChangeAButtonRight);
  buttonMapping[0]->UpdateFunction = UpdateButtonMapping;
  buttonMapping[1] = new MenuButton(&fontMenu, textureButtonBIconId, "", posX, posY += menuItemSize,
                                    OnClickChangeBButtonEnter, OnClickChangeBButtonLeft,
                                    OnClickChangeBButtonRight);

  SwapButtonSelectBack(swapButton);
  SwapButtonSelectBack(swapButton);
  MoveMenuButtonMapping(menuMappingButton, 0);
  MoveAButtonMapping(buttonMapping[0], 0);
  MoveBButtonMapping(buttonMapping[1], 0);

  buttonMapMenu.MenuItems.push_back(swapButton);
  buttonMapMenu.MenuItems.push_back(menuMappingButton);
  buttonMapMenu.MenuItems.push_back(buttonMapping[0]);
  buttonMapMenu.MenuItems.push_back(buttonMapping[1]);
  buttonMapMenu.MenuItems.push_back(new MenuButton(&fontMenu, textureBackIconId, "Back", posX,
                                                   posY += menuItemSize + bigGap, OnClickBackMove,
                                                   nullptr, nullptr));
  buttonMapMenu.BackPress = OnBackPressedMove;
  buttonMapMenu.Init();

  // button mapping overlay
  buttonMappingOverlay.MenuItems.push_back(new MenuImage(textureWhiteId,
                                                         0,
                                                         0,
                                                         menuWidth,
                                                         menuHeight,
                                                         {0.0f, 0.0f, 0.0f, 0.8f}));
  int overlayWidth = 250;
  int overlayHeight = 80;
  int margin = 15;
  buttonMappingOverlay.MenuItems.push_back(new MenuImage(textureWhiteId,
                                                         menuWidth / 2 - overlayWidth / 2,
                                                         menuHeight / 2 - overlayHeight / 2
                                                             - margin,
                                                         overlayWidth,
                                                         overlayHeight + margin * 2,
                                                         {0.05f, 0.05f, 0.05f, 0.3f}));

  mappingButtonLabel = new MenuLabel(&fontMenu,
                                     "A Button",
                                     menuWidth / 2 - overlayWidth / 2,
                                     menuHeight / 2 - overlayHeight / 2,
                                     overlayWidth,
                                     overlayHeight / 3,
                                     {0.9f, 0.9f, 0.9f, 0.9f});
  possibleMappingLabel = new MenuLabel(&fontMenu,
                                       "(A, B, X, Y)",
                                       menuWidth / 2 - overlayWidth / 2,
                                       menuHeight / 2 + overlayHeight / 2
                                           - overlayHeight / 3,
                                       overlayWidth,
                                       overlayHeight / 3,
                                       {0.9f, 0.9f, 0.9f, 0.9f});

  buttonMappingOverlay.MenuItems.push_back(mappingButtonLabel);
  buttonMappingOverlay.MenuItems.push_back(new MenuLabel(&fontMenu,
                                                         "Press Button",
                                                         menuWidth / 2 - overlayWidth / 2,
                                                         menuHeight / 2 - overlayHeight / 2
                                                             + overlayHeight / 3,
                                                         overlayWidth,
                                                         overlayHeight / 3,
                                                         {0.9f, 0.9f, 0.9f, 0.9f}));
  buttonMappingOverlay.MenuItems.push_back(possibleMappingLabel);

  // move menu page
  posY = HEADER_HEIGHT + 20;
  yawButton = new MenuButton(&fontMenu, texuterLeftRightIconId, "", posX, posY, OnClickYaw,
                             OnClickMoveScreenYawLeft, OnClickMoveScreenYawRight);
  yawButton->ScrollTimeH = 1;
  pitchButton =
      new MenuButton(&fontMenu, textureUpDownIconId, "", posX, posY += menuItemSize, OnClickPitch,
                     OnClickMoveScreenPitchLeft, OnClickMoveScreenPitchRight);
  pitchButton->ScrollTimeH = 1;
  rollButton =
      new MenuButton(&fontMenu, textureResetIconId, "", posX, posY += menuItemSize, OnClickRoll,
                     OnClickMoveScreenRollLeft, OnClickMoveScreenRollRight);
  rollButton->ScrollTimeH = 1;
  distanceButton = new MenuButton(&fontMenu,
                                  textureDistanceIconId,
                                  "",
                                  posX,
                                  posY += menuItemSize,
                                  OnClickDistance,
                                  OnClickMoveScreenDistanceLeft,
                                  OnClickMoveScreenDistanceRight);
  distanceButton->ScrollTimeH = 1;
  scaleButton =
      new MenuButton(&fontMenu, textureScaleIconId, "", posX, posY += menuItemSize, OnClickScale,
                     OnClickMoveScreenScaleLeft, OnClickMoveScreenScaleRight);
  scaleButton->ScrollTimeH = 1;

  moveMenu.MenuItems.push_back(yawButton);
  moveMenu.MenuItems.push_back(pitchButton);
  moveMenu.MenuItems.push_back(rollButton);
  moveMenu.MenuItems.push_back(distanceButton);
  moveMenu.MenuItems.push_back(scaleButton);

  moveMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                              textureResetViewIconId,
                                              "Reset View",
                                              posX,
                                              posY += menuItemSize + smallGap,
                                              OnClickResetView,
                                              nullptr,
                                              nullptr));
  moveMenu.MenuItems.push_back(new MenuButton(&fontMenu,
                                              textureBackIconId,
                                              "Back",
                                              posX,
                                              posY += menuItemSize + bigGap,
                                              OnClickBackMove,
                                              nullptr,
                                              nullptr));
  moveMenu.BackPress = OnBackPressedMove;
  moveMenu.Init();

  // updates the visible values
  MoveYaw(yawButton, 0);
  MovePitch(pitchButton, 0);
  MoveRoll(rollButton, 0);
  ChangeDistance(distanceButton, 0);
  ChangeScale(scaleButton, 0);

  currentMenu = &romSelectionMenu;
}