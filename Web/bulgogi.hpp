#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <jh/pod>


namespace bulgogi {
    namespace beast = boost::beast;
    namespace http = beast::http;
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

}

namespace views{
    /// @brief Check if the request method matches the expected method
    void check_head(const bulgogi::Request &req);
}

namespace bulgogi {

    [[maybe_unused]] inline bool is_json(const Request &req) {
        return req[http::field::content_type].starts_with("application/json");
    }

    inline void set_json(Response &res, const boost::json::value &value, int status_code = 200) {
        res.result(http::status(status_code));
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(value);
        res.prepare_payload();
    }

    inline void set_text(Response &res, std::string_view text, int status_code = 200) {
        res.result(http::status(status_code));
        res.set(http::field::content_type, "text/plain");
        res.body() = std::string(text);
        res.prepare_payload();
    }

    inline void set_html(Response &res, std::string_view html, int status_code = 200) {
        res.result(http::status(status_code));
        res.set(http::field::content_type, "text/html");
        res.body() = std::string(html);
        res.prepare_payload();
    }

    [[maybe_unused]] inline void set_binary(Response &res, std::string_view binary_data, const std::string &filename) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/octet-stream");
        res.set(http::field::content_disposition, "attachment; filename=\"" + filename + "\"");
        res.body() = std::string(binary_data);
        res.prepare_payload();
    }

    /**
     * Example usage:
     * @code
namespace download_types {
    constexpr jh::pod::array<char, 32> CSV  = {"csv"};
    constexpr jh::pod::array<char, 32> TSV  = {"tab-separated-values"};
    constexpr jh::pod::array<char, 32> YAML = {"yaml"};
    constexpr jh::pod::array<char, 32> HTML = {"html"};
    constexpr jh::pod::array<char, 32> PLAIN = {"plain"};
    constexpr jh::pod::array<char, 32> MD   = {"markdown"};
    constexpr jh::pod::array<char, 32> XML  = {"xml"};
}

using download_csv = bulgogi::set_download<download_types::CSV>;
using download_tsv = bulgogi::set_download<download_types::TSV>;
using download_yaml = bulgogi::set_download<download_types::YAML>;
using download_html = bulgogi::set_download<download_types::HTML>;
using download_plain = bulgogi::set_download<download_types::PLAIN>;
using download_md = bulgogi::set_download<download_types::MD>;
using download_xml = bulgogi::set_download<download_types::XML>;
     * @endcode
     */
    template<jh::pod::array<char, 32> Mime>
    struct [[maybe_unused]] set_download {
        [[maybe_unused]]
        static void apply(Response &res, std::string_view content, const std::string &filename) { // NOLINT
            res.result(http::status::ok);
            res.set(http::field::content_type, "text/" + std::string(Mime.data));
            res.set(http::field::content_disposition, "attachment; filename=\"" + filename + "\"");
            res.body() = std::string(content);
            res.prepare_payload();
        }
    };

    inline bool check_method(const Request &req, http::verb expected, Response &res) {
        if (req.method() != expected) {
            set_json(res, {
                    {"error",    "Method Not Allowed"},
                    {"expected", http::to_string(expected)},
                    {"got",      http::to_string(req.method())}
            }, 405);
            return false;
        }
        return true;
    }

    [[maybe_unused]] inline boost::json::object get_json_obj(const Request &req) {
        return boost::json::parse(req.body()).as_object();
    }

    [[maybe_unused]] inline std::string_view method_string(http::verb method) {
        return http::to_string(method);
    }

    [[maybe_unused]] inline void set_redirect(Response &res, const std::string &location, int code = 302) {
        res.result(static_cast<http::status>(code));
        res.set(http::field::location, location);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Redirecting to " + location;
        res.prepare_payload();
    }

    inline void apply_cors(const Request &req, bulgogi::Response &res, std::string_view allow_origin = "*") {
        try {
            views::check_head(req);
        } catch (const std::exception &e) {
            // Unauthorized
            set_text(res, std::string("Unauthorized: ") + e.what(), 401);

        }

        res.set(http::field::access_control_allow_origin, allow_origin);
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
        res.set(http::field::access_control_max_age, "86400");
    }

    [[maybe_unused]] inline std::optional<std::string> get_query_param(
            const boost::beast::http::request<boost::beast::http::string_body> &req,
            std::string_view key) {
        const std::string target = std::string(req.target());
        const std::string_view target_view = target;
        std::string_view query = target_view.substr(target.find('?') + 1); // query = view from owned string

        while (!query.empty()) {
            auto eq = query.find('=');
            if (eq == std::string_view::npos) break;

            std::string_view k = query.substr(0, eq);
            query.remove_prefix(eq + 1);

            auto amp = query.find('&');
            std::string_view v = (amp == std::string_view::npos) ? query : query.substr(0, amp);
            if (amp != std::string_view::npos) query.remove_prefix(amp + 1);
            else query = {};

            if (k == key) {
                return std::string(v); // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
                // safe: v from target_view, local variable
            }
        }
        return std::nullopt;
    }

}

