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


#include "sidepane.h"
#include <QComboBox>
#include <QVBoxLayout>
#include "placesview.h"
#include "dirtreeview.h"

namespace Fm {

SidePane::SidePane(QWidget* parent):
  QWidget(parent),
  showHidden_(false),
  mode_(ModeNone),
  view_(NULL),
  combo_(NULL),
  currentPath_(NULL),
  iconSize_(24, 24) {

  verticalLayout = new QVBoxLayout(this);
  verticalLayout->setContentsMargins(0, 0, 0, 0);

  combo_ = new QComboBox(this);
  combo_->setFrame(false);
  combo_->addItem(tr("Places"));
  combo_->addItem(tr("Directory Tree"));
  connect(combo_, SIGNAL(currentIndexChanged(int)), SLOT(onComboCurrentIndexChanged(int)));
  verticalLayout->addWidget(combo_);
}

SidePane::~SidePane() {
  if(currentPath_)
    fm_path_unref(currentPath_);
  // qDebug("delete SidePane");
}

void SidePane::onPlacesViewChdirRequested(int type, FmPath* path) {
  Q_EMIT chdirRequested(type, path);
}

void SidePane::onDirTreeViewChdirRequested(int type, FmPath* path) {
  Q_EMIT chdirRequested(type, path);
}

void SidePane::onComboCurrentIndexChanged(int current) {
  if(current != mode_) {
    setMode(Mode(current));
  }
}

void SidePane::setIconSize(QSize size) {
  iconSize_ = size;
  switch(mode_) {
    case ModePlaces:
    case ModeDirTree:
      static_cast<QTreeView*>(view_)->setIconSize(size);
      break;
  }
}

void SidePane::setCurrentPath(FmPath* path) {
  switch(mode_) {
    case ModePlaces:
      static_cast<PlacesView*>(view_)->setCurrentPath(path);
      break;
    case ModeDirTree:
      static_cast<DirTreeView*>(view_)->setCurrentPath(path);
      break;
  }
}

SidePane::Mode SidePane::modeByName(const char* str) {
  if(str == NULL)
    return ModeNone;
  if(strcmp(str, "places") == 0)
    return ModePlaces;
  if(strcmp(str, "dirtree") == 0)
    return ModeDirTree;
  return ModeNone;
}

const char* SidePane::modeName(SidePane::Mode mode) {
  switch(mode) {
  case ModePlaces:
    return "places";
  case ModeDirTree:
    return "dirtree";
  default:
    return NULL;
  }
}

#if 0 // FIXME: are these APIs from libfm-qt needed?

QString SidePane::modeLabel(SidePane::Mode mode) {
  switch(mode) {
  case ModePlaces:
    return tr("Places");
  case ModeDirTree:
    return tr("Directory Tree");
  }
  return QString();
}

QString SidePane::modeTooltip(SidePane::Mode mode) {
  switch(mode) {
  case ModePlaces:
    return tr("Shows list of common places, devices, and bookmarks in sidebar");
  case ModeDirTree:
    return tr("Shows tree of directories in sidebar");
  }
  return QString();
}
#endif

bool SidePane::setHomeDir(const char* home_dir) {
  if(view_ == NULL)
    return false;
  // TODO: SidePane::setHomeDir

  switch(mode_) {
  case ModePlaces:
    // static_cast<PlacesView*>(view_);
    return true;
  case ModeDirTree:
    // static_cast<PlacesView*>(view_);
    return true;
  }
  return true;
}

void SidePane::initDirTree() {
  // TODO
#if 0
  if(dir_tree_model)
      g_object_ref(dir_tree_model);
  else
  {
      FmFileInfoJob* job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_NONE);
      GList* l;
      /* query FmFileInfo for home dir and root dir, and then,
	* add them to dir tree model */
      fm_file_info_job_add(job, fm_path_get_home());
      fm_file_info_job_add(job, fm_path_get_root());

      /* FIXME: maybe it's cleaner to use run_async here? */
      GDK_THREADS_LEAVE();
      fm_job_run_sync_with_mainloop(FM_JOB(job));
      GDK_THREADS_ENTER();

      dir_tree_model = fm_dir_tree_model_new();
      for(l = fm_file_info_list_peek_head_link(job->file_infos); l; l = l->next)
      {
	  FmFileInfo* fi = FM_FILE_INFO(l->data);
	  fm_dir_tree_model_add_root(dir_tree_model, fi, NULL);
      }
      g_object_unref(job);

      g_object_add_weak_pointer((GObject*)dir_tree_model, (gpointer*)&dir_tree_model);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(sp->view), GTK_TREE_MODEL(dir_tree_model));
  g_object_unref(dir_tree_model);
#endif
}

void SidePane::setMode(Mode mode) {
  if(mode == mode_)
    return;

  if(view_) {
    delete view_;
    view_ = NULL;
    //if(sp->update_popup)
    //  g_signal_handlers_disconnect_by_func(sp->view, on_item_popup, sp);
  }
  mode_ = mode;

  combo_->setCurrentIndex(mode);
  switch(mode) {
  case ModePlaces: {
    PlacesView* placesView = new Fm::PlacesView(this);
    view_ = placesView;
    placesView->setCurrentPath(currentPath_);
    connect(placesView, SIGNAL(chdirRequested(int, FmPath*)), SLOT(onPlacesViewChdirRequested(int, FmPath*)));
    break;
  }
  case ModeDirTree:
    DirTreeView* dirTreeView = new Fm::DirTreeView(this);
    view_ = dirTreeView;
    initDirTree();
    dirTreeView->setCurrentPath(currentPath_);
    connect(dirTreeView, SIGNAL(chdirRequested(int, FmPath*)), SLOT(onDirTreeViewChdirRequested(int, FmPath*)));
    break;
  }
  if(view_) {
    // if(sp->update_popup)
    //  g_signal_connect(sp->view, "item-popup", G_CALLBACK(on_item_popup), sp);
    verticalLayout->addWidget(view_);
  }
  Q_EMIT modeChanged();
}

bool SidePane::setShowHidden(bool show_hidden) {
  if(view_ == NULL)
    return false;
  // TODO: SidePane::setShowHidden

  // spec = g_object_class_find_property(klass, "show-hidden");
  //if(spec == NULL || spec->value_type != G_TYPE_BOOLEAN)
  //  return false; /* isn't supported by view */
  // g_object_set(sp->view, "show-hidden", show_hidden, NULL);
  return true;
}

} // namespace Fm
