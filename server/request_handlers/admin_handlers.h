#ifndef ADMIN_HANDLERS_H
#define ADMIN_HANDLERS_H

#include "../session_manager.h"
#include "../json_utils.h"
#include <string>

namespace MillionaireGame {

/**
 * Admin request handlers
 * Handles ADD_QUES, CHANGE_QUES, VIEW_QUES, GET_QUESTION, DEL_QUES,
 * BAN_USER, VIEW_USERS, PROMOTE_USER, REVOKE_ADMIN
 */
namespace AdminHandlers {
    std::string handleAddQues(const std::string& request, ClientSession& session, int client_fd);
    std::string handleChangeQues(const std::string& request, ClientSession& session, int client_fd);
    std::string handleViewQues(const std::string& request, ClientSession& session, int client_fd);
    std::string handleGetQuestion(const std::string& request, ClientSession& session, int client_fd);
    std::string handleDelQues(const std::string& request, ClientSession& session, int client_fd);
    std::string handleBanUser(const std::string& request, ClientSession& session, int client_fd);
    std::string handleViewUsers(const std::string& request, ClientSession& session, int client_fd);
    std::string handlePromoteUser(const std::string& request, ClientSession& session, int client_fd);
    std::string handleRevokeAdmin(const std::string& request, ClientSession& session, int client_fd);
}

} // namespace MillionaireGame

#endif // ADMIN_HANDLERS_H

