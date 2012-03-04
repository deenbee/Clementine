CREATE TABLE podcasts (
  url TEXT,
  title TEXT,
  description TEXT,
  copyright TEXT,
  link TEXT,
  image_url TEXT,
  author TEXT,
  owner_name TEXT,
  owner_email TEXT,

  extra BLOB
);

CREATE TABLE podcast_episodes (
  podcast_id INTEGER,

  title TEXT,
  description TEXT,
  author TEXT,
  publication_date INTEGER,
  duration_secs INTEGER,
  url TEXT,

  listened BOOLEAN,
  downloaded BOOLEAN,
  local_url TEXT,

  extra BLOB
);

CREATE INDEX podcast_idx_url ON podcasts(url);

CREATE INDEX podcast_episodes_idx_podcast_id ON podcast_episodes(podcast_id);

UPDATE schema_version SET version=37;
