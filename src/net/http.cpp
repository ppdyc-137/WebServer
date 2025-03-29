#include "http.h"

#include <sstream>

namespace sylar {

    std::string HttpRequest::toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();

    }

    std::string HttpResponse::toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    std::ostream& HttpRequest::dump(std::ostream& os) const {
        os << httpMethodToString(method_) << " "
           << path_
           << (query_.empty() ? "" : "?") << query_
           << (fragment_.empty() ? "" : "#") << fragment_
           << " HTTP/"
           << (version_ >> 4) << "." << (version_ & 0x0F)
           << "\r\n";
        if(!websocket_) {
            os << "connection: " << (closed_ ? "close" : "keep-alive") << "\r\n";
        }
        for(const auto& header : headers_) {
            if(!websocket_ && strcasecmp(header.first.c_str(), "connection") == 0) {
                continue;
            }
            os << header.first << ": " << header.second << "\r\n";
        }

        if(!body_.empty()) {
            os << "content-length: " << body_.size() << "\r\n\r\n"
               << body_;
        } else {
            os << "\r\n";
        }
        return os;
    }

    std::ostream& HttpResponse::dump(std::ostream& os) const {
        os << "HTTP/"
           << (version_ >> 4) << "." << (version_ & 0x0F)
           << " " << static_cast<uint16_t>(status_)
           << " " << httpStatusToString(status_)
           << "\r\n";
        for(const auto& header: headers_) {
            if (!websocket_ && strcasecmp(header.first.c_str(), "connection") == 0) {
                continue;
            }
            os << header.first << ": " << header.second << "\r\n";
        }
        for(const auto& cookie: cookies_) {
            os << "Set-Cookie: " << cookie << "\r\n";
        }
        if (!websocket_) {
            os << "connection: " << (closed_ ? "close" : "keep-alive") << "\r\n";
        }
        if (!body_.empty()) {
            os << "content-length: " << body_.size() << "\r\n\r\n"
                << body_;
        } else {
            os << "\r\n";
        }
        return os;
    }

}
