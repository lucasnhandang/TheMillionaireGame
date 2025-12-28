#ifndef LIFELINE_MANAGER_H
#define LIFELINE_MANAGER_H

#include <string>

namespace MillionaireGame {

/**
 * Lifeline Result Structure
 */
struct LifelineResult {
    bool success;
    std::string lifeline_type;  // "5050", "PHONE", "AUDIENCE"
    int delay_seconds;  // Delay before showing result
    std::string result_data;  // JSON string with lifeline result
    
    LifelineResult() : success(false), delay_seconds(0) {}
};

/**
 * Lifeline Manager
 * Handles lifeline processing (50/50, Phone a Friend, Ask the Audience)
 */
class LifelineManager {
public:
    static LifelineManager& getInstance();
    
    /**
     * Use 50/50 lifeline - removes 2 incorrect answers
     * @param game_id Game session ID
     * @param question_id Question ID
     * @return LifelineResult with remaining options
     */
    LifelineResult use5050(int game_id, int question_id);
    
    /**
     * Use Phone a Friend lifeline - get suggestion from friend
     * @param game_id Game session ID
     * @param question_id Question ID
     * @return LifelineResult with friend's suggestion
     */
    LifelineResult usePhone(int game_id, int question_id);
    
    /**
     * Use Ask the Audience lifeline - get audience poll results
     * @param game_id Game session ID
     * @param question_id Question ID
     * @return LifelineResult with audience percentages
     */
    LifelineResult useAudience(int game_id, int question_id);
    
    /**
     * Check if a lifeline has been used
     * @param game_id Game session ID
     * @param lifeline_type Lifeline type ("5050", "PHONE", "AUDIENCE")
     * @return true if already used
     */
    bool isLifelineUsed(int game_id, const std::string& lifeline_type);

private:
    LifelineManager() = default;
    ~LifelineManager() = default;
    LifelineManager(const LifelineManager&) = delete;
    LifelineManager& operator=(const LifelineManager&) = delete;
};

} // namespace MillionaireGame

#endif // LIFELINE_MANAGER_H

