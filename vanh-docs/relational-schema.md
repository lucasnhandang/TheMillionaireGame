erDiagram
  USERS {
    INT      user_id          "PK"
    STRING   username
    STRING   password_hash
    DATETIME created_at
    STRING   role
  }

  GAMES {
    INT      game_id          "PK"
    INT      user_id          "FK → USERS.user_id"
    DATETIME started_at
    DATETIME ended_at
    INT      current_q_number
    INT      current_amount
    STRING   status
    BOOLEAN  used_5050
    BOOLEAN  used_ask
    BOOLEAN  used_phone
  }

  QUESTIONS {
    INT      question_id      "PK"
    STRING   question_text
    STRING   option_a
    STRING   option_b
    STRING   option_c
    STRING   option_d
    STRING   correct_answer
    STRING   difficulty
    BOOLEAN  is_active
    DATETIME created_at
  }

  GAME_QUESTIONS {
    INT      game_id          "PK, FK → GAMES.game_id"
    INT      question_order   "PK"
    INT      question_id      "FK → QUESTIONS.question_id"
  }

  GAME_ANSWERS {
    INT      game_id          "PK, FK → GAMES.game_id"
    INT      question_order   "PK  (FK → GAME_QUESTIONS.question_order)"
    DATETIME submitted_at
    STRING   selected_option
    BOOLEAN  is_correct
    INT      response_time_second
  }

  %% NEW: per-user stats / leaderboard
  LEADERBOARD {
    INT      user_id               "PK, FK → USERS.user_id"
    INT      total_games_played
    INT      total_correct_answers
    INT      total_amount_earned
    INT      best_amount
    DATETIME last_played_at
  }

  %% NEW: friendships
  FRIENDSHIPS {
    INT      user_id               "PK, FK → USERS.user_id"
    INT      friend_user_id        "PK, FK → USERS.user_id"
    STRING   status                "pending/accepted/blocked"
    DATETIME created_at
    DATETIME accepted_at
  }

  %% NEW: direct messages / chat
  MESSAGES {
    INT      message_id            "PK"
    INT      sender_id             "FK → USERS.user_id"
    INT      receiver_id           "FK → USERS.user_id"
    INT      game_id               "FK → GAMES.game_id (nullable)"
    DATETIME sent_at
    STRING   content
    BOOLEAN  is_read
  }

  %% Relationships / cardinalities
  USERS ||--o{ GAMES         : plays
  GAMES ||--|{ GAME_QUESTIONS: contains
  QUESTIONS ||--o{ GAME_QUESTIONS : selected_as
  GAMES ||--o{ GAME_ANSWERS  : records
  GAME_QUESTIONS ||--o| GAME_ANSWERS : answered_as

  %% New relationships
  USERS ||--o| LEADERBOARD    : has_stats

  USERS ||--o{ FRIENDSHIPS   : as_user
  USERS ||--o{ FRIENDSHIPS   : as_friend

  USERS ||--o{ MESSAGES      : sends
  USERS ||--o{ MESSAGES      : receives
