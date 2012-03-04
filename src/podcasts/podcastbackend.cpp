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

#include "podcastbackend.h"
#include "core/application.h"
#include "core/database.h"
#include "core/scopedtransaction.h"

#include <QMutexLocker>

PodcastBackend::PodcastBackend(Application* app, QObject* parent)
  : QObject(parent),
    app_(app),
    db_(app->database())
{
}

void PodcastBackend::Subscribe(Podcast* podcast) {
  // If this podcast is already in the database, do nothing
  if (podcast->is_valid()) {
    return;
  }

  // If there's an entry in the database with the same URL, take its data.
  Podcast existing_podcast = GetSubscriptionByUrl(podcast->url());
  if (existing_podcast.is_valid()) {
    *podcast = existing_podcast;
    return;
  }

  QMutexLocker l(db_->Mutex());
  QSqlDatabase db(db_->Connect());
  ScopedTransaction t(&db);

  // Insert the podcast.
  QSqlQuery q("INSERT INTO podcasts (" + Podcast::kColumnSpec + ")"
              " VALUES " + Podcast::kBindSpec, db);
  podcast->BindToQuery(&q);

  q.exec();
  if (db_->CheckErrors(q))
    return;

  // Update the database ID.
  const int database_id = q.lastInsertId().toInt();
  podcast->set_database_id(database_id);

  // Update the IDs of any episodes.
  PodcastEpisodeList* episodes = podcast->mutable_episodes();
  for (PodcastEpisodeList::iterator it = episodes->begin() ; it != episodes->end() ; ++it) {
    it->set_podcast_database_id(database_id);
  }

  // Add those episodes to the database.
  AddEpisodes(episodes, &db);
}

void PodcastBackend::AddEpisodes(PodcastEpisodeList* episodes, QSqlDatabase* db) {
  QSqlQuery q("INSERT INTO podcast_episodes (" + PodcastEpisode::kColumnSpec + ")"
              " VALUES " + PodcastEpisode::kBindSpec, *db);

  for (PodcastEpisodeList::iterator it = episodes->begin() ; it != episodes->end() ; ++it) {
    it->BindToQuery(&q);
    q.exec();
    if (db_->CheckErrors(q))
      return;

    const int database_id = q.lastInsertId().toInt();
    it->set_database_id(database_id);
  }
}

PodcastList PodcastBackend::GetAllSubscriptions() {
  PodcastList ret;

  QMutexLocker l(db_->Mutex());
  QSqlDatabase db(db_->Connect());

  QSqlQuery q("SELECT ROWID, " + Podcast::kColumnSpec +
              " FROM podcasts", db);

  q.exec();
  if (db_->CheckErrors(q))
    return ret;

  while (q.next()) {
    Podcast podcast;
    podcast.InitFromQuery(q);
    ret << podcast;
  }

  return ret;
}

Podcast PodcastBackend::GetSubscriptionById(int id) {
  Podcast ret;

  QMutexLocker l(db_->Mutex());
  QSqlDatabase db(db_->Connect());

  QSqlQuery q("SELECT ROWID, " + Podcast::kColumnSpec +
              " FROM podcasts"
              " WHERE ROWID = :id", db);

  q.bindValue(":id", id);
  q.exec();
  if (!db_->CheckErrors(q) && q.next()) {
    ret.InitFromQuery(q);
  }

  return ret;
}

Podcast PodcastBackend::GetSubscriptionByUrl(const QUrl& url) {
  Podcast ret;

  QMutexLocker l(db_->Mutex());
  QSqlDatabase db(db_->Connect());

  QSqlQuery q("SELECT ROWID, " + Podcast::kColumnSpec +
              " FROM podcasts"
              " WHERE url = :url", db);

  q.bindValue(":url", url.toEncoded());
  q.exec();
  if (!db_->CheckErrors(q) && q.next()) {
    ret.InitFromQuery(q);
  }

  return ret;
}
