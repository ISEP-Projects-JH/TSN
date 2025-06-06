#include "views.hpp"
#include "bulgogi.hpp"
#include "../Application/UserModelHandler.hpp"
#include "../Application/FabricInfoHandler.hpp"
#include "../Application/Business.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <random>
#include <mysql/mysql.h>
#include <thread>

namespace json = boost::json;
using bulgogi::Request; /// @brief HTTP request
using bulgogi::Response; /// @brief HTTP response
using bulgogi::check_method; /// @brief Check HTTP method
using bulgogi::set_json; /// @brief Set JSON response

std::mt19937 &global_rng() {
    static std::mt19937 rng([] {
        std::random_device rd;
        return std::mt19937(rd() ^ static_cast<unsigned>(time(nullptr)));
    }());
    return rng;
}


static MYSQL *g_mysql_conn = nullptr;
static std::unique_ptr<social::UserModelHandler> g_user_handler{};
static std::unique_ptr<fabric::FabricInfoHandler> g_fabric_handler{};

inline bool ensure_mysql_ready(bulgogi::Response &res, MYSQL *conn) {
    if (!conn || !g_user_handler || !g_fabric_handler) {
        set_json(res, {{
                               "error",   "MySQL not connected or handlers uninitialized"},
                       {       "missing", {
                                                  {"conn", conn == nullptr},
                                                  {"user_handler", g_user_handler == nullptr},
                                                  {"fabric_handler", g_fabric_handler == nullptr}
                                          }},
                       {       "hint",    "POST /api/set_db_connection to reinitialize"
                       }}, 500);
        return false; // means early return
    }
    return true;
}

/// @brief Global function map for registered urls
std::unordered_map<std::string, views::HandlerFunc> views::function_map;

/// @brief Atomic boolean to signal server shutdown
extern std::atomic<bool> g_should_exit;

/// @brief Global acceptor for TCP connections
extern std::unique_ptr<boost::asio::ip::tcp::acceptor> global_acceptor;

void views::init() {
    g_mysql_conn = mysql_init(nullptr);
    if (!g_mysql_conn) {
        throw std::runtime_error("mysql_init failed");
    }
}


void views::atexit() {
    g_user_handler.reset();
    g_fabric_handler.reset();

    if (g_mysql_conn) {
        mysql_close(g_mysql_conn);
        g_mysql_conn = nullptr;
        std::cout << "[Exit] MySQL connection closed.\n";
    }
}


void views::check_head([[maybe_unused]] const bulgogi::Request &req) {
//    if (req.find("Origin") != req.end()) {
//        std::string origin = std::string(req["Origin"]);
//        if (origin.empty()) {
//            throw std::runtime_error("CORS blocked: origin not allowed");
//        }
//    }
//
//    // Allow all common headers for development
//    if (req.find("Access-Control-Request-Headers") != req.end()) {
//        std::string headers = std::string(req["Access-Control-Request-Headers"]);
//        // Don't block unless explicitly forbidden
//        if (headers.find("Content-Type") == std::string::npos &&
//            headers.find("Authorization") == std::string::npos) {
//            throw std::runtime_error("CORS preflight failed: required header not allowed");
//        }
//    }
// dev: pass all
}


REGISTER_VIEW(ping) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    set_json(res, {{"status", "alive"}});
}

REGISTER_VIEW(shutdown_server) {
    if (!check_method(req, bulgogi::http::verb::post, res)) return;

    if (!g_should_exit.exchange(true)) {
        std::cout << "Called Exit\n";

        if (global_acceptor && global_acceptor->is_open()) {
            boost::system::error_code ec;
            global_acceptor->cancel(ec); // NOLINT
        }

        try {
            boost::asio::io_context ioc;
            boost::asio::ip::tcp::socket s(ioc);
            s.connect({boost::asio::ip::address_v4::loopback(), PORT});
        } catch (...) {}
    }

    set_json(res, {{"status", "server_shutdown_requested"}});
}

REGISTER_VIEW(api, simulate_day) {
    if (!check_method(req, bulgogi::http::verb::post, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    if (!g_user_handler || !g_fabric_handler) {
        set_json(res, {{"error", "Handlers not ready"}}, 500);
        return;
    }

    auto result = fabric::api::next_day(*g_user_handler, *g_fabric_handler);
    set_json(res, {
            {"new_users",          result.new_users},
            {"new_friendships",    result.new_friendships},
            {"total_interactions", result.total_interactions}
    });
}

REGISTER_VIEW(api, get_user_profile) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    auto params = bulgogi::get_query_param(req, "id");
    if (!params) {
        set_json(res, {{"error", "Missing user ID"}}, 400);
        return;
    }

    uint32_t user_id = std::stoul(*params);
    try {
        auto profile = fabric::api::get_user_profile(user_id, *g_user_handler, *g_fabric_handler);
        set_json(res, fabric::api::to_json(profile, user_id));
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, get_user_profile_simple) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    auto params = bulgogi::get_query_param(req, "id");
    if (!params) {
        set_json(res, {{"error", "Missing user ID"}}, 400);
        return;
    }

    uint32_t user_id = std::stoul(*params);
    try {
        auto simple_profile = fabric::api::get_user_simple_profile(user_id, *g_fabric_handler);
        set_json(res, fabric::api::simple_json(simple_profile, user_id));
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, refresh_db) {
    if (!check_method(req, bulgogi::http::verb::post, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    if (!g_user_handler || !g_fabric_handler) {
        set_json(res, {{"error", "Handlers not ready"}}, 500);
        return;
    }

    try {
        fabric::api::clear_all(*g_user_handler, *g_fabric_handler);
        uint32_t new_user_count = fabric::api::initialize_population(*g_user_handler, *g_fabric_handler);
        set_json(res, {{"status",    "database_refreshed"},
                       {"new_users", new_user_count}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, random_user_id) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    if (!g_fabric_handler) {
        set_json(res, {{"error", "Fabric handler not ready"}}, 500);
        return;
    }

    try {
        uint32_t user_id = fabric::api::random_user_id(*g_fabric_handler);
        set_json(res, {{"user_id", user_id}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, get_total_count) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    if (!g_fabric_handler) {
        set_json(res, {{"error", "Fabric handler not ready"}}, 500);
        return;
    }

    try {
        uint32_t total_count = g_fabric_handler->get_count();
        set_json(res, {{"total_users", total_count}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, batch_get_simple_profiles) {
    if (!check_method(req, bulgogi::http::verb::post, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    auto body = json::parse(req.body());
    if (!body.is_array()) {
        set_json(res, {{"error", "Invalid request format"}}, 400);
        return;
    }

    boost::json::array user_ids = body.as_array();
    boost::json::array profiles_json;

    try {
        for (const auto &id: user_ids) {
            if (!id.is_int64()) continue; // Skip invalid IDs
            uint32_t user_id = id.as_int64();
            auto simple_profile = fabric::api::get_user_simple_profile(user_id, *g_fabric_handler);
            profiles_json.emplace_back(fabric::api::simple_json(simple_profile, user_id));
        }
        set_json(res, {{"profiles", profiles_json}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, recommend_fof) {
    // use social::recommend_a_star
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    auto params = bulgogi::get_query_param(req, "id");
    if (!params) {
        set_json(res, {{"error", "Missing user ID"}}, 400);
        return;
    }
    uint32_t user_id = std::stoul(*params);
    try {
        const auto user = g_user_handler->load_user_by_id(user_id);
        auto result = social::recommend_A_star<64>(user, *g_user_handler);
        boost::json::array recommendations_json;
        for (const jh::pod::pod_like auto &id: result) {
            if (id == INVALID_FRIEND_ID) continue; // Skip invalid entries
            recommendations_json.emplace_back(id);
        }
        set_json(res, {{"recommendations", recommendations_json}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }

}

REGISTER_VIEW(api, recommend_strangers) {
    // use social::recommend_strangers
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    auto params = bulgogi::get_query_param(req, "id");
    if (!params) {
        set_json(res, {{"error", "Missing user ID"}}, 400);
        return;
    }
    uint32_t user_id = std::stoul(*params);
    try {
        auto user = g_user_handler->load_user_by_id(user_id);
        auto recommendations = social::recommend_strangers<20>(user, *g_user_handler);
        boost::json::array recommendations_json;
        for (const jh::pod::pod_like auto &id: recommendations) {
            if (id != INVALID_FRIEND_ID) {
                recommendations_json.emplace_back(id);
            } else break; // Reach end of valid recommendations
        }
        set_json(res, {{"recommendations", recommendations_json}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}

REGISTER_VIEW(api, get_user_friends) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    if (!ensure_mysql_ready(res, g_mysql_conn)) return;

    auto params = bulgogi::get_query_param(req, "id");
    if (!params) {
        set_json(res, {{"error", "Missing user ID"}}, 400);
        return;
    }

    uint32_t user_id = std::stoul(*params);
    try {
        auto friends = fabric::api::get_user_friends(user_id, *g_user_handler, *g_fabric_handler);
        boost::json::array friends_json;
        for (uint32_t fid: friends) {
            friends_json.emplace_back(fid);
        }
        set_json(res, {{"friends", friends_json}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}


const char *db_source = R"__db_src__(
-- Temporarily disable foreign key checks to prevent dependency errors during creation
SET FOREIGN_KEY_CHECKS = 0;

-- Drop old tables if exist
DROP TABLE IF EXISTS `UserModels`;
DROP TABLE IF EXISTS `UsersFabric`;

-- Create parent table: UsersFabric
CREATE TABLE `UsersFabric` (
    `user_id`       BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `first_name_id` INT UNSIGNED NOT NULL,
    `last_name_id`  INT UNSIGNED NOT NULL,
    `avatar_id`     INT UNSIGNED NOT NULL,
    PRIMARY KEY (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci AUTO_INCREMENT = 1;

-- Create child table: UserModels (with inline foreign key)
CREATE TABLE `UserModels` (
    `user_id`      BIGINT UNSIGNED NOT NULL,
    `interests_16` BIGINT UNSIGNED NOT NULL,
    `base_64_bits` BIGINT UNSIGNED NOT NULL,
    `friends`      BLOB NOT NULL,
    PRIMARY KEY (`user_id`),
    CONSTRAINT `fk_user_id` FOREIGN KEY (`user_id`)
        REFERENCES `UsersFabric` (`user_id`)
        ON DELETE CASCADE
        ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- Restore FK checks
SET FOREIGN_KEY_CHECKS = 1;

)__db_src__";

REGISTER_VIEW(api, set_db_connection) {
    if (!check_method(req, bulgogi::http::verb::post, res)) return;

    auto body = json::parse(req.body());
    auto host = json::value_to<std::string>(body.at("host"));
    auto user = json::value_to<std::string>(body.at("user"));
    auto password = json::value_to<std::string>(body.at("password"));
    auto dbname = json::value_to<std::string>(body.at("database"));
    auto port = json::value_to<uint32_t>(body.at("port"));

    try {
        // Disconnect
        views::atexit();

        auto renew = bulgogi::get_query_param(req, "renew");

        // Reconnect with new config
        g_mysql_conn = mysql_init(nullptr);
        if (!mysql_real_connect(g_mysql_conn, host.c_str(), user.c_str(), password.c_str(),
                                dbname.c_str(), port, nullptr, 0)) {
            throw std::runtime_error(mysql_error(g_mysql_conn));
        }

        if (renew && *renew == "true") {
            std::istringstream ss(db_source);
            std::string line, statement;
            int sql_id = 0;

            std::cout << "====== Begin Executing SQL Script ======" << std::endl;
            while (std::getline(ss, line)) {
                if (line.empty() || line.starts_with("--")) continue;

                statement += line + "\n";
                if (line.find(';') != std::string::npos) {
                    sql_id++;
                    std::cout << "[SQL #" << sql_id << "] Executing:\n" << statement << std::endl;

                    if (mysql_query(g_mysql_conn, statement.c_str()) != 0) {
                        std::cerr << "❌ SQL Error at statement #" << sql_id << ": " << mysql_error(g_mysql_conn) << std::endl;
                        throw std::runtime_error(mysql_error(g_mysql_conn));
                    }

                    std::cout << "✔️ SQL OK\n" << std::endl;
                    statement.clear();
                }
            }
            std::cout << "====== SQL Script Executed Successfully ======" << std::endl;
        }

        // Wait to be prepared
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        g_user_handler = std::make_unique<social::UserModelHandler>(g_mysql_conn);
        g_fabric_handler = std::make_unique<fabric::FabricInfoHandler>(g_mysql_conn);

        set_json(res, {{"status", "connected"},
                       {"db",     dbname}});
    } catch (const std::exception &e) {
        set_json(res, {{"error", e.what()}}, 500);
    }
}
