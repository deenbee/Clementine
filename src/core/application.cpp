/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "application.h"
#include "appearance.h"
#include "database.h"
#include "player.h"
#include "tagreaderclient.h"
#include "taskmanager.h"
#include "covers/albumcoverloader.h"
#include "covers/coverproviders.h"
#include "covers/currentartloader.h"
#include "devices/devicemanager.h"
#include "internet/internetmodel.h"
#include "globalsearch/globalsearch.h"
#include "library/library.h"
#include "library/librarybackend.h"
#include "playlist/playlistbackend.h"
#include "playlist/playlistmanager.h"
#include "podcasts/podcastbackend.h"

Application::Application(QObject* parent)
  : QObject(parent),
    tag_reader_client_(NULL),
    database_(NULL),
    album_cover_loader_(NULL),
    appearance_(NULL),
    cover_providers_(NULL),
    task_manager_(NULL),
    player_(NULL),
    playlist_manager_(NULL),
    current_art_loader_(NULL),
    global_search_(NULL),
    internet_model_(NULL),
    library_(NULL),
    playlist_backend_(NULL),
    device_manager_(NULL),
    podcast_backend_(NULL)
{
  tag_reader_client_ = new TagReaderClient(this);
  MoveToNewThread(tag_reader_client_);
  tag_reader_client_->Start();

  database_ = new Database(this, this);
  MoveToNewThread(database_);

  album_cover_loader_ = new AlbumCoverLoader(this);
  MoveToNewThread(album_cover_loader_);

  appearance_ = new Appearance(this);
  cover_providers_ = new CoverProviders(this);
  task_manager_ = new TaskManager(this);
  player_ = new Player(this, this);
  playlist_manager_ = new PlaylistManager(this, this);
  current_art_loader_ = new CurrentArtLoader(this, this);
  global_search_ = new GlobalSearch(this, this);
  internet_model_ = new InternetModel(this, this);

  library_ = new Library(this, this);

  playlist_backend_ = new PlaylistBackend(this, this);
  MoveToThread(playlist_backend_, database_->thread());

  device_manager_ = new DeviceManager(this, this);

  podcast_backend_ = new PodcastBackend(this, this);
  MoveToThread(podcast_backend_, database_->thread());

  library_->Init();
  library_->StartThreads();
}

Application::~Application() {
  // It's important that the device manager is deleted before the database.
  // Deleting the database deletes all objects that have been created in its
  // thread, including some device library backends.
  delete device_manager_; device_manager_ = NULL;

  foreach (QObject* object, objects_in_threads_) {
    object->deleteLater();
  }

  foreach (QThread* thread, threads_) {
    thread->quit();
  }

  foreach (QThread* thread, threads_) {
    thread->wait();
  }
}

void Application::MoveToNewThread(QObject* object) {
  QThread* thread = new QThread(this);

  MoveToThread(object, thread);

  thread->start();
  threads_ << thread;
}

void Application::MoveToThread(QObject* object, QThread* thread) {
  object->setParent(NULL);
  object->moveToThread(thread);
  objects_in_threads_ << object;
}

void Application::AddError(const QString& message) {
  emit ErrorAdded(message);
}

LibraryBackend* Application::library_backend() const {
  return library()->backend();
}

LibraryModel* Application::library_model() const {
  return library()->model();
}
