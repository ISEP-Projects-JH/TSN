#pragma once

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <sstream>
#include <atomic>
#include "../Utils/Fabric.hpp"

namespace fabric {

    class FabricInfoHandler {
    public:
        explicit FabricInfoHandler(MYSQL* db) : conn(db) {
            if (!conn) {
                throw std::runtime_error("MySQL connection is null");
            }
            update_count(); // Initialize count
        }

        [[nodiscard]] UsersFabric load_user(uint64_t id) const {
            const std::string query = "SELECT first_name_id, last_name_id, avatar_id FROM UsersFabric WHERE user_id = ? LIMIT 1";
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND param[1]{};
            param[0].buffer_type = MYSQL_TYPE_LONGLONG;
            param[0].buffer = &id;
            param[0].is_unsigned = true;

            if (mysql_stmt_bind_param(stmt, param) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            uint32_t first_name_id, last_name_id, avatar_id;
            MYSQL_BIND result[3]{};

            result[0].buffer_type = MYSQL_TYPE_LONG;
            result[0].buffer = &first_name_id;
            result[0].is_unsigned = true;

            result[1].buffer_type = MYSQL_TYPE_LONG;
            result[1].buffer = &last_name_id;
            result[1].is_unsigned = true;

            result[2].buffer_type = MYSQL_TYPE_LONG;
            result[2].buffer = &avatar_id;
            result[2].is_unsigned = true;

            if (mysql_stmt_bind_result(stmt, result) != 0 ||
                mysql_stmt_execute(stmt) != 0 ||
                mysql_stmt_store_result(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_fetch(stmt) != 0)
                throw std::runtime_error("UsersFabric entry not found");

            mysql_stmt_close(stmt);

            return UsersFabric{id, first_name_id, last_name_id, avatar_id};
        }

        [[maybe_unused]] void insert_user(const UsersFabric& user) {
            std::lock_guard lock(mut);

            const std::string query =
                    "INSERT IGNORE INTO UsersFabric (user_id, first_name_id, last_name_id, avatar_id) VALUES (?, ?, ?, ?)";
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND bind[4]{};

            bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[0].buffer = (void*)&user.user_id;
            bind[0].is_unsigned = true;

            bind[1].buffer_type = MYSQL_TYPE_LONG;
            bind[1].buffer = (void*)&user.first_name_id;
            bind[1].is_unsigned = true;

            bind[2].buffer_type = MYSQL_TYPE_LONG;
            bind[2].buffer = (void*)&user.last_name_id;
            bind[2].is_unsigned = true;

            bind[3].buffer_type = MYSQL_TYPE_LONG;
            bind[3].buffer = (void*)&user.avatar_id;
            bind[3].is_unsigned = true;

            if (mysql_stmt_bind_param(stmt, bind) != 0 ||
                mysql_stmt_execute(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            mysql_stmt_close(stmt);
            update_count(); // refresh count
        }

        void batch_insert_users(const std::vector<UsersFabric>& users) {
            if (users.empty()) return;
            std::lock_guard lock(mut);

            std::ostringstream sql;
            sql << "INSERT IGNORE INTO UsersFabric (user_id, first_name_id, last_name_id, avatar_id) VALUES ";

            bool first = true;
            for (const auto& user : users) {
                if (!first) {
                    sql << ", ";
                } else {
                    first = false;
                }
                sql << "("
                    << user.user_id << ", "
                    << user.first_name_id << ", "
                    << user.last_name_id << ", "
                    << user.avatar_id << ")";
            }

            if (mysql_query(conn, sql.str().c_str()) != 0)
                throw std::runtime_error("Failed batch insert: " + std::string(mysql_error(conn)));

            update_count(); // refresh count
        }

        void clear_all() {
            std::lock_guard lock(mut);
            mysql_query(conn, "SET FOREIGN_KEY_CHECKS = 0");

            const char* query = "TRUNCATE TABLE `UsersFabric`";
            if (mysql_query(conn, query) != 0) {
                throw std::runtime_error("Failed to clear UsersFabric: " + std::string(mysql_error(conn)));
            }

            mysql_query(conn, "SET FOREIGN_KEY_CHECKS = 1");
            count = 0;
        }

        [[nodiscard]] uint32_t get_count() const noexcept {
            return count.load();
        }

        [[nodiscard]] std::vector<UsersFabric> batch_load_users_by_ids(const std::vector<uint32_t>& ids) const {
            if (ids.empty()) return {};

            std::ostringstream query;
            query << "SELECT user_id, first_name_id, last_name_id, avatar_id FROM UsersFabric WHERE user_id IN (";
            for (size_t i = 0; i < ids.size(); ++i) {
                if (i != 0) query << ", ";
                query << ids[i];
            }
            query << ")";

            if (mysql_query(conn, query.str().c_str()) != 0)
                throw std::runtime_error("Failed batch load: " + std::string(mysql_error(conn)));

            MYSQL_RES* res = mysql_store_result(conn);
            if (!res) throw std::runtime_error("Failed to store result from batch load");

            std::vector<UsersFabric> results;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                UsersFabric user{};
                user.user_id = static_cast<uint32_t>(std::stoul(row[0]));
                user.first_name_id = static_cast<uint32_t>(std::stoul(row[1]));
                user.last_name_id = static_cast<uint32_t>(std::stoul(row[2]));
                user.avatar_id = static_cast<uint32_t>(std::stoul(row[3]));
                results.emplace_back(user);
            }

            mysql_free_result(res);
            return results;
        }


    private:
        MYSQL* conn;
        mutable std::mutex mut;
        std::atomic<uint32_t> count;

        void update_count() {
            const char* query = "SELECT COUNT(*) FROM UsersFabric";
            if (mysql_query(conn, query) != 0) {
                throw std::runtime_error("Failed to count UsersFabric: " + std::string(mysql_error(conn)));
            }

            MYSQL_RES* res = mysql_store_result(conn);
            if (!res) throw std::runtime_error("Failed to store result from count");

            MYSQL_ROW row = mysql_fetch_row(res);
            if (!row) {
                mysql_free_result(res);
                throw std::runtime_error("Failed to fetch count row");
            }

            count = static_cast<uint32_t>(std::stoul(row[0]));
            mysql_free_result(res);
        }
    };

} // namespace fabric
