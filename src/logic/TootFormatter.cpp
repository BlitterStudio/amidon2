#include "MastodonTypes.h"
#include "TootFormatter.h"

namespace TootFormatter {

std::string FormatToot(const Status& status) {
    std::string html = "<html><body>";
    html += "<b>" + status.author_displayname + "</b>";
    html += " <i>@" + status.author_username + "</i><br><br>";
    html += status.content;
    html += "</body></html>";
    return html;
}

}
