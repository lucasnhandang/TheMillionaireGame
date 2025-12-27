#include "connection_handlers.h"
#include <ctime>

using namespace std;

namespace MillionaireGame {

namespace ConnectionHandlers {

string handlePing(const string& request, ClientSession& session) {
    string data = "{\"message\":\"PONG\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleConnection(const string& request, ClientSession& session) {
    string data = "{\"message\":\"Connection alive\",\"timestamp\":" + to_string(time(nullptr)) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace ConnectionHandlers

} // namespace MillionaireGame

