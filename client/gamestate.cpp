#include "gamestate.h"

// Prize ladder in USD (matching gamescreen.ui display)
const int GameState::PRIZE_LADDER[15] = {
    100,      // Q1: $100
    200,      // Q2: $200
    300,      // Q3: $300
    500,      // Q4: $500
    1000,     // Q5: $1,000 (checkpoint)
    2000,     // Q6: $2,000
    4000,     // Q7: $4,000
    8000,     // Q8: $8,000
    16000,    // Q9: $16,000
    32000,    // Q10: $32,000 (checkpoint)
    64000,    // Q11: $64,000
    125000,   // Q12: $125,000
    250000,   // Q13: $250,000
    500000,   // Q14: $500,000
    1000000   // Q15: $1,000,000 (checkpoint - $1 MILLION)
};
