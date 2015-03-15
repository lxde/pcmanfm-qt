/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "settings.h"
#include <QDir>
#include <QFile>
#include <QStringBuilder>
#include <QSettings>
#include <QApplication>
#include "desktopwindow.h"
#include "utilities.h"
// #include <QDesktopServices>

using namespace PCManFM;

inline static const char* bookmarkOpenMethodToString(int value);
inline static int bookmarkOpenMethodFromString(const QString str);

inline static const char* wallpaperModeToString(int value);
inline static int wallpaperModeFromString(const QString str);

inline static const char* viewModeToString(Fm::FolderView::ViewMode value);
inline static Fm::FolderView::ViewMode viewModeFromString(const QString str);

inline static const char* sidePaneModeToString(int value);
inline static int sidePaneModeFromString(const QString str);

inline static const char* sortOrderToString(Qt::SortOrder order);
inline static Qt::SortOrder sortOrderFromString(const QString str);

inline static const char* sortColumnToString(Fm::FolderModel::ColumnId value);
inline static Fm::FolderModel::ColumnId sortColumnFromString(const QString str);

Settings::Settings():
  QObject(),
  supportTrash_(Fm::isUriSchemeSupported("trash")),
  fallbackIconThemeName_(),
  useFallbackIconTheme_(QIcon::themeName().isEmpty() || QIcon::themeName() == "hicolor"),
  bookmarkOpenMethod_(0),
  suCommand_(),
  terminal_(),
  mountOnStartup_(true),
  mountRemovable_(true),
  autoRun_(true),
  closeOnUnmount_(false),
  wallpaperMode_(0),
  wallpaper_(),
  desktopBgColor_(),
  desktopFgColor_(),
  desktopShadowColor_(),
  showWmMenu_(false),
  desktopShowHidden_(false),
  desktopSortOrder_(Qt::AscendingOrder),
  desktopSortColumn_(Fm::FolderModel::ColumnFileName),
  alwaysShowTabs_(true),
  showTabClose_(true),
  fixedWindowWidth_(640),
  fixedWindowHeight_(480),
  lastWindowWidth_(640),
  lastWindowHeight_(480),
  lastWindowMaximized_(false),
  splitterPos_(120),
  sidePaneMode_(0),
  viewMode_(Fm::FolderView::IconMode),
  showHidden_(false),
  sortOrder_(Qt::AscendingOrder),
  sortColumn_(Fm::FolderModel::ColumnFileName),
  // settings for use with libfm
  singleClick_(false),
  autoSelectionDelay_(600),
  useTrash_(true),
  confirmTrash_(false),
  confirmDelete_(true),
  noUsbTrash_(false),
  showThumbnails_(true),
  archiver_(),
  siUnit_(false),
  bigIconSize_(48),
  smallIconSize_(24),
  sidePaneIconSize_(24),
  thumbnailIconSize_(128) {
}

Settings::~Settings() {

}

QString Settings::profileDir(QString profile, bool useFallback) {
  // NOTE: it's a shame that QDesktopServices does not handle XDG_CONFIG_HOME
  // try user-specific config file first
  QString dirName = QLatin1String(qgetenv("XDG_CONFIG_HOME"));
  if (dirName.isEmpty())
    dirName = QDir::homePath() % QLatin1String("/.config");
  dirName = dirName % "/pcmanfm-qt/" % profile;
  QDir dir(dirName);

  // if user config dir does not exist, try system-wide config dirs instead
  if(!dir.exists() && useFallback) {
    QString fallbackDir;
    for(const char* const* configDir = g_get_system_config_dirs(); *configDir; ++configDir) {
      fallbackDir = QString(*configDir) % "/pcmanfm-qt/" % profile;
      dir.setPath(fallbackDir);
      if(dir.exists()) {
	dirName = fallbackDir;
	break;
      }
    }
  }
  return dirName;
}

bool Settings::load(QString profile) {
  profileName_ = profile;
  QString fileName = profileDir(profile, true) % "/settings.conf";
  return loadFile(fileName);
}

bool Settings::save(QString profile) {
  QString fileName = profileDir(profile.isEmpty() ? profileName_ : profile) % "/settings.conf";
  return saveFile(fileName);
}

bool Settings::loadFile(QString filePath) {
  QSettings settings(filePath, QSettings::IniFormat);
  settings.beginGroup("System");
  fallbackIconThemeName_ = settings.value("FallbackIconThemeName").toString();
  if(fallbackIconThemeName_.isEmpty()) {
    // FIXME: we should choose one from installed icon themes or get
    // the value from XSETTINGS instead of hard code a fallback value.
    fallbackIconThemeName_ = "elementary"; // fallback icon theme name
  }
  suCommand_ = settings.value("SuCommand", "gksu %s").toString();
  setTerminal(settings.value("Terminal", "xterm").toString());
  setArchiver(settings.value("Archiver", "file-roller").toString());
  setSiUnit(settings.value("SIUnit", false).toBool());
  settings.endGroup();

  settings.beginGroup("Behavior");
  bookmarkOpenMethod_ = bookmarkOpenMethodFromString(settings.value("BookmarkOpenMethod").toString());
  // settings for use with libfm
  useTrash_ = settings.value("UseTrash", true).toBool();
  singleClick_ = settings.value("SingleClick", false).toBool();
  autoSelectionDelay_ = settings.value("AutoSelectionDelay", 600).toInt();
  confirmTrash_ = settings.value("ConfirmTrash", false).toBool();
  confirmDelete_ = settings.value("ConfirmDelete", true).toBool();
  noUsbTrash_ = settings.value("NoUsbTrash", false).toBool();
  fm_config->no_usb_trash = noUsbTrash_; // also set this to libfm since FmFileOpsJob reads this config value before trashing files.
  // bool thumbnailLocal_;
  // bool thumbnailMax;
  settings.endGroup();

  settings.beginGroup("Desktop");
  wallpaperMode_ = wallpaperModeFromString(settings.value("WallpaperMode").toString());
  wallpaper_ = settings.value("Wallpaper").toString();
  desktopBgColor_.setNamedColor(settings.value("BgColor", "#000000").toString());
  desktopFgColor_.setNamedColor(settings.value("FgColor", "#ffffff").toString());
  desktopShadowColor_.setNamedColor(settings.value("ShadowColor", "#000000").toString());
  if(settings.contains("Font"))
    desktopFont_.fromString(settings.value("Font").toString());
  else
    desktopFont_ = QApplication::font();
  showWmMenu_ = settings.value("ShowWmMenu", false).toBool();
  desktopShowHidden_ = settings.value("ShowHidden", false).toBool();

  desktopSortOrder_ = sortOrderFromString(settings.value("SortOrder").toString());
  desktopSortColumn_ = sortColumnFromString(settings.value("SortColumn").toString());
  settings.endGroup();

  settings.beginGroup("Volume");
  mountOnStartup_ = settings.value("MountOnStartup", true).toBool();
  mountRemovable_ = settings.value("MountRemovable", true).toBool();
  autoRun_ = settings.value("AutoRun", true).toBool();
  closeOnUnmount_ = settings.value("CloseOnUnmount", true).toBool();
  settings.endGroup();

  settings.beginGroup("Thumbnail");
  showThumbnails_ = settings.value("ShowThumbnails", true).toBool();
  setMaxThumbnailFileSize(settings.value("MaxThumbnailFileSize", 4096).toInt());
  setThumbnailLocalFilesOnly(settings.value("ThumbnailLocalFilesOnly", true).toBool());
  settings.endGroup();

  settings.beginGroup("FolderView");
  viewMode_ = viewModeFromString(settings.value("Mode", Fm::FolderView::IconMode).toString());
  showHidden_ = settings.value("ShowHidden", false).toBool();
  sortOrder_ = sortOrderFromString(settings.value("SortOrder").toString());
  sortColumn_ = sortColumnFromString(settings.value("SortColumn").toString());

  // override config in libfm's FmConfig
  bigIconSize_ = settings.value("BigIconSize", 48).toInt();
  smallIconSize_ = settings.value("SmallIconSize", 24).toInt();
  sidePaneIconSize_ = settings.value("SidePaneIconSize", 24).toInt();
  thumbnailIconSize_ = settings.value("ThumbnailIconSize", 128).toInt();
  settings.endGroup();

  settings.beginGroup("Window");
  fixedWindowWidth_ = settings.value("FixedWidth", 640).toInt();
  fixedWindowHeight_ = settings.value("FixedHeight", 480).toInt();
  lastWindowWidth_ = settings.value("LastWindowWidth", 640).toInt();
  lastWindowHeight_ = settings.value("LastWindowHeight", 480).toInt();
  lastWindowMaximized_ = settings.value("LastWindowMaximized", false).toBool();
  rememberWindowSize_ = settings.value("RememberWindowSize", true).toBool();
  alwaysShowTabs_ = settings.value("AlwaysShowTabs", true).toBool();
  showTabClose_ = settings.value("ShowTabClose", true).toBool();
  splitterPos_ = settings.value("SplitterPos", 150).toInt();
  sidePaneMode_ = sidePaneModeFromString(settings.value("SidePaneMode").toString());
  settings.endGroup();

  return true;
}

bool Settings::saveFile(QString filePath) {
  QSettings settings(filePath, QSettings::IniFormat);

  settings.beginGroup("System");
  settings.setValue("FallbackIconThemeName", fallbackIconThemeName_);
  settings.setValue("SuCommand", suCommand_);
  settings.setValue("Terminal", terminal_);
  settings.setValue("Archiver", archiver_);
  settings.setValue("SIUnit", siUnit_);
  settings.endGroup();

  settings.beginGroup("Behavior");
  // settings.setValue("BookmarkOpenMethod", bookmarkOpenMethodToString(bookmarkOpenMethod_));
  // settings for use with libfm
  settings.setValue("UseTrash", useTrash_);
  settings.setValue("SingleClick", singleClick_);
  settings.setValue("AutoSelectionDelay", autoSelectionDelay_);

  settings.setValue("ConfirmTrash", confirmTrash_);
  settings.setValue("ConfirmDelete", confirmDelete_);
  settings.setValue("NoUsbTrash", noUsbTrash_);
  // bool thumbnailLocal_;
  // bool thumbnailMax;
  settings.endGroup();

  settings.beginGroup("Desktop");
  settings.setValue("WallpaperMode", wallpaperModeToString(wallpaperMode_));
  settings.setValue("Wallpaper", wallpaper_);
  settings.setValue("BgColor", desktopBgColor_.name());
  settings.setValue("FgColor", desktopFgColor_.name());
  settings.setValue("ShadowColor", desktopShadowColor_.name());
  settings.setValue("Font", desktopFont_.toString());
  settings.setValue("ShowWmMenu", showWmMenu_);
  settings.setValue("ShowHidden", desktopShowHidden_);
  settings.setValue("SortOrder", sortOrderToString(desktopSortOrder_));
  settings.setValue("SortColumn", sortColumnToString(desktopSortColumn_));
  settings.endGroup();

  settings.beginGroup("Volume");
  settings.setValue("MountOnStartup", mountOnStartup_);
  settings.setValue("MountRemovable", mountRemovable_);
  settings.setValue("AutoRun", autoRun_);
  settings.setValue("CloseOnUnmount", closeOnUnmount_);
  settings.endGroup();

  settings.beginGroup("Thumbnail");
  settings.setValue("ShowThumbnails", showThumbnails_);
  settings.setValue("MaxThumbnailFileSize", maxThumbnailFileSize());
  settings.setValue("ThumbnailLocalFilesOnly", thumbnailLocalFilesOnly());
  settings.endGroup();

  settings.beginGroup("FolderView");
  settings.setValue("Mode", viewModeToString(viewMode_));
  settings.setValue("ShowHidden", showHidden_);
  settings.setValue("SortOrder", sortOrderToString(sortOrder_));
  settings.setValue("SortColumn", sortColumnToString(sortColumn_));

  // override config in libfm's FmConfig
  settings.setValue("BigIconSize", bigIconSize_);
  settings.setValue("SmallIconSize", smallIconSize_);
  settings.setValue("SidePaneIconSize", sidePaneIconSize_);
  settings.setValue("ThumbnailIconSize", thumbnailIconSize_);
  settings.endGroup();

  settings.beginGroup("Window");
  settings.setValue("FixedWidth", fixedWindowWidth_);
  settings.setValue("FixedHeight", fixedWindowHeight_);
  settings.setValue("LastWindowWidth", lastWindowWidth_);
  settings.setValue("LastWindowHeight", lastWindowHeight_);
  settings.setValue("LastWindowMaximized", lastWindowMaximized_);
  settings.setValue("RememberWindowSize", rememberWindowSize_);
  settings.setValue("AlwaysShowTabs", alwaysShowTabs_);
  settings.setValue("ShowTabClose", showTabClose_);
  settings.setValue("SplitterPos", splitterPos_);
  // settings.setValue("SidePaneMode", sidePaneModeToString(sidePaneMode_));
  settings.endGroup();
  return true;
}

static const char* bookmarkOpenMethodToString(int value) {
  return "";
}

static int bookmarkOpenMethodFromString(const QString str) {
  return 0;
}

static const char* viewModeToString(Fm::FolderView::ViewMode value) {
  const char* ret;
  switch(value) {
    case Fm::FolderView::IconMode:
    default:
      ret = "icon";
      break;
    case Fm::FolderView::CompactMode:
      ret = "compact";
      break;
    case Fm::FolderView::DetailedListMode:
      ret = "detailed";
      break;
    case Fm::FolderView::ThumbnailMode:
      ret = "thumbnail";
      break;
  }
  return ret;
}

Fm::FolderView::ViewMode viewModeFromString(const QString str) {
  Fm::FolderView::ViewMode ret;
  if(str == "icon")
    ret = Fm::FolderView::IconMode;
  else if(str == "compact")
    ret = Fm::FolderView::CompactMode;
  else if(str == "detailed")
    ret = Fm::FolderView::DetailedListMode;
  else if(str == "thumbnail")
    ret = Fm::FolderView::ThumbnailMode;
  else
    ret = Fm::FolderView::IconMode;
  return ret;
}

static const char* sortOrderToString(Qt::SortOrder order) {
  return (order == Qt::DescendingOrder ? "descending" : "ascending");
}

static Qt::SortOrder sortOrderFromString(const QString str) {
  return (str == "descending" ? Qt::DescendingOrder : Qt::AscendingOrder);
}

static const char* sortColumnToString(Fm::FolderModel::ColumnId value) {
  const char* ret;
  switch(value) {
    case Fm::FolderModel::ColumnFileName:
    default:
      ret = "name";
      break;
    case Fm::FolderModel::ColumnFileType:
      ret = "type";
      break;
    case Fm::FolderModel::ColumnFileSize:
      ret = "size";
      break;
    case Fm::FolderModel::ColumnFileMTime:
      ret = "mtime";
      break;
    case Fm::FolderModel::ColumnFileOwner:
      ret = "owner";
      break;
  }
  return ret;
}

static Fm::FolderModel::ColumnId sortColumnFromString(const QString str) {
  Fm::FolderModel::ColumnId ret;
  if(str == "name")
      ret = Fm::FolderModel::ColumnFileName;
  else if(str == "type")
    ret = Fm::FolderModel::ColumnFileType;
  else if(str == "size")
    ret = Fm::FolderModel::ColumnFileSize;
  else if(str == "mtime")
    ret = Fm::FolderModel::ColumnFileMTime;
  else if(str == "owner")
    ret = Fm::FolderModel::ColumnFileOwner;
  else
    ret = Fm::FolderModel::ColumnFileName;
  return ret;
}

static const char* wallpaperModeToString(int value) {
  const char* ret;
  switch(value) {
    case DesktopWindow::WallpaperNone:
    default:
      ret = "none";
      break;
    case DesktopWindow::WallpaperStretch:
      ret = "stretch";
      break;
    case DesktopWindow::WallpaperFit:
      ret = "fit";
      break;
    case DesktopWindow::WallpaperCenter:
      ret = "center";
      break;
    case DesktopWindow::WallpaperTile:
      ret = "tile";
      break;
  }
  return ret;
}

static int wallpaperModeFromString(const QString str) {
  int ret;
  if(str == "stretch")
    ret = DesktopWindow::WallpaperStretch;
  else if(str == "fit")
    ret = DesktopWindow::WallpaperFit;
  else if(str == "center")
    ret = DesktopWindow::WallpaperCenter;
  else if(str == "tile")
    ret = DesktopWindow::WallpaperTile;
  else
    ret = DesktopWindow::WallpaperNone;
  return ret;
}

static const char* sidePaneModeToString(int value) {
  return NULL;
}

static int sidePaneModeFromString(const QString str) {
  return 0;
}

void Settings::setTerminal(QString terminalCommand) {
    terminal_ = terminalCommand;
    // override the settings in libfm FmConfig.
    g_free(fm_config->terminal);
    fm_config->terminal = g_strdup(terminal_.toLocal8Bit().constData());
    g_signal_emit_by_name(fm_config, "changed::terminal");
  }
