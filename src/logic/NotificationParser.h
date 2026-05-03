#ifndef LOGIC_NOTIFICATION_PARSER_H
#define LOGIC_NOTIFICATION_PARSER_H

#include "MastodonTypes.h"

#include <string>
#include <vector>

namespace NotificationParser {

std::vector<Notification> ParseNotifications(const std::string& json);

}

#endif // LOGIC_NOTIFICATION_PARSER_H
