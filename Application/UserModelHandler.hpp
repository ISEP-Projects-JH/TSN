#pragma once

#include <mysql/mysql.h>
#include <string>
#include <stdexcept>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include "../Entities/UserModel.hpp"

using interaction_batch = pod::array<pod::pair<uint32_t, uint32_t>, 256>;

namespace social {
    JH_POD_STRUCT(UserProfileView,
        uint32_t id;
        uint64_t interests_16;
        uint64_t base_64_bits;
    );

    class UserModelHandler {
    public:
        explicit UserModelHandler(MYSQL *db) : conn(db) {
        }

        /// Load user from db
        [[nodiscard]] UserModel load_user_by_id(const uint32_t user_id) const {
            UserModel user{};
            const std::string query = "SELECT interests_16, base_64_bits, friends FROM UserModels WHERE user_id = ? LIMIT 1";

            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND param[1]{};
            param[0].buffer_type = MYSQL_TYPE_LONG;
            param[0].buffer = (void *) &user_id; // NOLINT mysql expression
            param[0].is_unsigned = true;

            if (mysql_stmt_bind_param(stmt, param) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            // prepare output buffers
            uint64_t interests;
            uint64_t base_bits;
            unsigned long blob_len = 0;
            std::array<std::byte, sizeof(friend_list)> buffer{};

            MYSQL_BIND result[3]{};
            result[0].buffer_type = MYSQL_TYPE_LONGLONG;
            result[0].buffer = &interests;
            result[0].is_unsigned = true;

            result[1].buffer_type = MYSQL_TYPE_LONGLONG;
            result[1].buffer = &base_bits;
            result[1].is_unsigned = true;

            result[2].buffer_type = MYSQL_TYPE_BLOB;
            result[2].buffer = buffer.data();
            result[2].buffer_length = buffer.size();
            result[2].length = &blob_len;

            if (mysql_stmt_bind_result(stmt, result) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_execute(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_store_result(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_fetch(stmt) != 0)
                throw std::runtime_error("UserModel not found or fetch failed");

            mysql_stmt_close(stmt);

            user.user_id = user_id;
            user.interests_16 = interests;
            user.base_64_bits = base_bits;
            if (blob_len != sizeof(friend_list))
                throw std::runtime_error("Invalid blob length");

            user.friends = jh::pod::bytes_view{buffer.data(), blob_len}.clone<friend_list>();

            return user;
        }

        [[maybe_unused]] void public_save_user(const UserModel &user) {
            std::lock_guard lock(mut);
            save_user(user);
        }

        [[nodiscard]] pod::array<uint32_t, 256> get_friend_ids(const uint32_t id) const {
            pod::array<uint32_t, 256> result{};
            auto [user_id, interests_16, base_64_bits, friends] = load_user_by_id(id);

            size_t idx = 0;
            for (const auto &[first, _]: friends) {
                if (first != INVALID_FRIEND_ID && idx < 256) {
                    result[idx++] = first;
                }
            }
            return result;
        }

        [[maybe_unused]] void update_interests_16(uint32_t user_id, uint64_t new_val) const {
            const std::string query = "UPDATE UserModels SET interests_16 = ? WHERE user_id = ?";
            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND bind[2]{};
            constexpr bool one = true;

            bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[0].buffer = (void *) &new_val; // NOLINT mysql expression
            bind[0].is_unsigned = one;

            bind[1].buffer_type = MYSQL_TYPE_LONG;
            bind[1].buffer = (void *) &user_id; // NOLINT mysql expression
            bind[1].is_unsigned = one;

            if (mysql_stmt_bind_param(stmt, bind) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_execute(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            mysql_stmt_close(stmt);
        }

        /// Check if two UserModels are friends
        [[maybe_unused]] [[nodiscard]] bool is_friend(const uint32_t id, const uint32_t target_id) const {
            auto [user_id, interests_16, base_64_bits, friends] = load_user_by_id(id);
            return std::any_of(friends.begin(), friends.end(), [&](const auto &pair) {
                return pair.first == target_id;
            });
        }

        /// Add a new user with given interests and base bits
        [[maybe_unused]] void create_user(const uint32_t id, const uint64_t interests_16, const uint64_t base_64_bits) const {
            const std::string query = "INSERT INTO UserModels (user_id, interests_16, base_64_bits, friends) "
                                      "SELECT ?, ?, ?, ? FROM DUAL WHERE NOT EXISTS "
                                      "(SELECT 1 FROM UserModels WHERE user_id = ?)";

            friend_list empty_list{};
            const auto view = jh::pod::bytes_view::from(empty_list);
            auto blob_len = static_cast<unsigned long>(view.len);

            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND bind[5]{};
            constexpr bool one = true;

            bind[0].buffer_type = MYSQL_TYPE_LONG;
            bind[0].buffer = (void *) &id; // NOLINT
            bind[0].is_unsigned = one;

            bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[1].buffer = (void *) &interests_16; // NOLINT
            bind[1].is_unsigned = one;

            bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[2].buffer = (void *) &base_64_bits; // NOLINT
            bind[2].is_unsigned = one;

            bind[3].buffer_type = MYSQL_TYPE_BLOB;
            bind[3].buffer = const_cast<std::byte *>(view.data);
            bind[3].buffer_length = blob_len;
            bind[3].length = &blob_len;

            bind[4].buffer_type = MYSQL_TYPE_LONG;
            bind[4].buffer = (void *) &id;   // NOLINT
            bind[4].is_unsigned = one;

            if (mysql_stmt_bind_param(stmt, bind) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_execute(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            mysql_stmt_close(stmt);
        }

        [[maybe_unused]] void update_base_64_bits(uint32_t user_id, uint64_t new_val) const {
            const std::string query = "UPDATE UserModels SET base_64_bits = ? WHERE user_id = ?";
            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND bind[2]{};
            constexpr bool one = true;

            bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[0].buffer = (void *) &new_val; // NOLINT mysql expression
            bind[0].is_unsigned = one;

            bind[1].buffer_type = MYSQL_TYPE_LONG;
            bind[1].buffer = (void *) &user_id; // NOLINT mysql expression
            bind[1].is_unsigned = one;

            if (mysql_stmt_bind_param(stmt, bind) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_execute(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            mysql_stmt_close(stmt);
        }

        /// Batch load users by IDs
        [[nodiscard]]
        std::unordered_map<uint32_t, UserModel>
        batch_load_users_by_ids(const std::unordered_set<uint32_t>& ids) const {
            std::unordered_map<uint32_t, UserModel> result;
            if (ids.empty()) return result;

            std::ostringstream oss;
            oss << "SELECT user_id, interests_16, base_64_bits, friends FROM UserModels WHERE user_id IN (";
            bool first = true;
            for (uint32_t id : ids) {
                if (!first) oss << ',';
                oss << id;
                first = false;
            }
            oss << ")";

            const std::string query = oss.str();
            if (mysql_query(conn, query.c_str()) != 0) {
                throw std::runtime_error(std::string("MySQL batch query failed: ") + mysql_error(conn));
            }

            MYSQL_RES* res = mysql_store_result(conn);
            if (!res) throw std::runtime_error("mysql_store_result() failed");

            MYSQL_ROW row;
            unsigned long* lengths;

            while ((row = mysql_fetch_row(res))) {
                lengths = mysql_fetch_lengths(res);
                if (!lengths) continue;

                uint32_t user_id = static_cast<uint32_t>(std::stoul(row[0]));
                uint64_t interests_16 = std::stoull(row[1]);
                uint64_t base_64_bits = std::stoull(row[2]);

                if (lengths[3] != sizeof(friend_list)) {
                    // corrupted blob
                    continue;
                }

                friend_list friends{};
                std::memcpy(&friends, row[3], sizeof(friend_list));

                UserModel user{};
                user.user_id = user_id;
                user.interests_16 = interests_16;
                user.base_64_bits = base_64_bits;
                user.friends = friends;

                result[user_id] = user;
            }

            mysql_free_result(res);
            return result;
        }


        [[nodiscard]] UserProfileView get_user_profile_view(const uint32_t id) const {
            const std::string query = "SELECT interests_16, base_64_bits FROM UserModels WHERE user_id = " +
                                      std::to_string(id) + " LIMIT 1";

            if (mysql_query(conn, query.c_str()) != 0) {
                throw std::runtime_error(mysql_error(conn));
            }

            MYSQL_RES *result = mysql_store_result(conn);
            if (!result) throw std::runtime_error("Query failed: no result");

            char *const *row = mysql_fetch_row(result);
            if (!row) {
                mysql_free_result(result);
                throw std::runtime_error("UserModel not found");
            }

            UserProfileView view{};
            view.id = id;
            view.interests_16 = std::stoull(row[0]);
            view.base_64_bits = std::stoull(row[1]);

            mysql_free_result(result);
            return view;
        }

        [[nodiscard]]
        std::unordered_map<uint32_t, UserProfileView>
        batch_get_user_profile_views(const std::unordered_set<uint32_t>& ids) const {
            std::unordered_map<uint32_t, UserProfileView> result;
            if (ids.empty()) return result;

            std::ostringstream oss;
            oss << "SELECT user_id, interests_16, base_64_bits FROM UserModels WHERE user_id IN (";
            bool first = true;
            for (uint32_t id : ids) {
                if (!first) oss << ',';
                oss << id;
                first = false;
            }
            oss << ")";

            const std::string query = oss.str();
            if (mysql_query(conn, query.c_str()) != 0) {
                throw std::runtime_error(std::string("MySQL batch query failed: ") + mysql_error(conn));
            }

            MYSQL_RES* res = mysql_store_result(conn);
            if (!res) throw std::runtime_error("mysql_store_result() failed");

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                if (!row[0] || !row[1] || !row[2]) continue;

                UserProfileView view{};
                view.id = static_cast<uint32_t>(std::stoul(row[0]));
                view.interests_16 = std::stoull(row[1]);
                view.base_64_bits = std::stoull(row[2]);

                result[view.id] = view; // full copy
            }

            mysql_free_result(res);
            return result;
        }

        bool add_interaction(const uint32_t user_id, const uint32_t target_id, const uint32_t amount = 1) {
            std::lock_guard lock(mut);

            UserModel user = load_user_by_id(user_id);
            social::add_interaction(user, target_id, amount);
            save_user(user);
            return true;
        }

        [[maybe_unused]] bool add_interactions(const uint32_t user_id, const interaction_batch &interactions) {
            std::lock_guard lock(mut);

            UserModel user = load_user_by_id(user_id);
            for (const auto &[fid, amt]: interactions) {
                if (fid != INVALID_FRIEND_ID && amt > 0) {
                    social::add_interaction(user, fid, amt);
                }
            }
            save_user(user);
            return true;
        }

        bool add_friend(const uint32_t id1, const uint32_t id2, const uint32_t score = 0) {
            std::lock_guard lock(mut);

            UserModel u1 = load_user_by_id(id1);
            if (!social::add_friend(u1, id2, score)) return false;
            save_user(u1);
            return true;
        }

        bool remove_friend(const uint32_t id1, const uint32_t id2) {
            std::lock_guard lock(mut);

            UserModel u1 = load_user_by_id(id1);
            if (!social::remove_friend(u1, id2)) return false;
            save_user(u1);
            return true;
        }

        void decay_interactions(const uint32_t id1, const float rate = 0.95f) {
            std::lock_guard lock(mut);

            UserModel u1 = load_user_by_id(id1);
            social::decay_interactions(u1, rate);
            save_user(u1);
        }

        static std::string build_interest_similarity_sql(const uint32_t user_id,
                                                         const uint64_t user_interest,
                                                         const uint8_t high_threshold = 7,
                                                         const uint8_t mid_threshold = 5,
                                                         const size_t num = 200) {
            std::ostringstream oss;
            oss << "SELECT user_id FROM UserModels WHERE user_id != " << user_id;

            bool first = true;
            for (int i = 0; i < 16; ++i) {
                if (const uint8_t val = (user_interest >> (i * 4)) & 0xF; val >= high_threshold) {
                    oss << " AND ((interests_16 >> " << (i * 4) << ") & 0xF) >= " << static_cast<int>(mid_threshold);
                    first = false;
                }
            }

            if (first) return ""; // No interests
            oss << " ORDER BY RAND() LIMIT " << num;
            return oss.str();
        }

        template<uint16_t N>
        pod::array<uint32_t, N> build_interest_similarity(const UserModel &self,
                                                          const uint8_t high_threshold = 7,
                                                          const uint8_t mid_threshold = 5) const {
            pod::array<uint32_t, N> result{};
            const std::string query = build_interest_similarity_sql(self.user_id, self.interests_16,
                                                                    high_threshold, mid_threshold, N);
            if (query.empty()) return result;

            if (mysql_query(conn, query.c_str()) != 0)
                throw std::runtime_error(mysql_error(conn));

            MYSQL_RES *res = mysql_store_result(conn);
            if (!res) throw std::runtime_error("Query failed: no result");

            MYSQL_ROW row;
            size_t idx = 0;
            while (((row = mysql_fetch_row(res))) && idx < N) {
                result[idx++] = static_cast<uint32_t>(std::stoul(row[0]));
            }
            mysql_free_result(res);
            return result;
        }

        static inline std::string hex_encode(const std::byte *data, size_t len) {
            static constexpr char hex[] = "0123456789ABCDEF";
            std::vector<char> buf;
            buf.reserve(len * 2);  // Pre-allocate exact space

            for (size_t i = 0; i < len; ++i) {
                auto byte = static_cast<uint8_t>(data[i]);
                buf.emplace_back(hex[byte >> 4]);
                buf.emplace_back(hex[byte & 0x0F]);
            }
            return {buf.data(), buf.size()}; // Construct string from vector
        }

        void batch_insert_users(const std::vector<UserModel> &users) const {
            if (users.empty()) return;
            std::lock_guard lock(mut);

            std::ostringstream sql;
            sql << "INSERT INTO UserModels (user_id, interests_16, base_64_bits, friends) VALUES ";

            bool first = true;
            for (const auto &user: users) {
                const auto view = jh::pod::bytes_view::from(user.friends);
                const std::string hex_blob = hex_encode(view.data, view.len); // safe BLOB to hex

                if (!first) {
                    sql << ", ";
                } else {
                    first = false;
                }

                sql << "("
                    << user.user_id << ", "
                    << user.interests_16 << ", "
                    << user.base_64_bits << ", "
                    << "UNHEX('" << hex_blob << "'))";
            }

            // Optional: use ON DUPLICATE to update existing user
            sql << " ON DUPLICATE KEY UPDATE "
                << "interests_16=VALUES(interests_16), "
                << "base_64_bits=VALUES(base_64_bits), "
                << "friends=VALUES(friends)";

            const std::string query = sql.str();
            if (mysql_query(conn, query.c_str()) != 0) {
                throw std::runtime_error(std::string("Failed batch insert: ") + mysql_error(conn));
            }
        }

        void clear_user_table() const {
            std::lock_guard lock(mut);
            const std::string query = "TRUNCATE TABLE UserModels";
            if (mysql_query(conn, query.c_str()) != 0) {
                throw std::runtime_error("Failed to clear UserModels: " + std::string(mysql_error(conn)));
            }
        }


#ifdef DEBUG

        /// Add a pair of friends, real logic should be modified by Client-end business logic
        bool add_friend_pair(const uint32_t id1, const uint32_t id2) {
            std::lock_guard lock(mut);

            User u1 = load_user_by_id(id1);
            User u2 = load_user_by_id(id2);

            if (!add_friend_mutual(u1, u2)) return false;
            save_user(u1);
            save_user(u2);
            return true;
        }

        /// Remove a pair of friends, real logic should be modified by Client-end business logic
        bool remove_friend_pair(const uint32_t id1, const uint32_t id2) {
            std::lock_guard lock(mut);

            User u1 = load_user_by_id(id1);
            User u2 = load_user_by_id(id2);

            if (!remove_friend_mutual(u1, u2)) return false;

            save_user(u1);
            save_user(u2);
            return true;
        }


        void clear_all_users() const {
            std::lock_guard lock(mut);
            const std::string query = "DELETE FROM UserModels";
            if (mysql_query(conn, query.c_str()) != 0) {
                throw std::runtime_error(std::string("Failed to clear users: ") + mysql_error(conn));
            }
        }

#endif

    private:
        MYSQL *conn;
        mutable std::mutex mut;

        /// Renew
        void save_user(const UserModel &user) {
            const std::string query =
                    "REPLACE INTO UserModels (user_id, interests_16, base_64_bits, friends) VALUES (?, ?, ?, ?)";
            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("stmt_init() failed");

            if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            MYSQL_BIND bind[4]{};
            constexpr bool one = true;

            bind[0].buffer_type = MYSQL_TYPE_LONG;
            bind[0].buffer = (void *) &user.user_id; // NOLINT mysql expression
            bind[0].is_unsigned = one;

            bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[1].buffer = (void *) &user.interests_16; // NOLINT mysql expression
            bind[1].is_unsigned = one;

            bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[2].buffer = (void *) &user.base_64_bits; // NOLINT mysql expression
            bind[2].is_unsigned = one;

            bind[3].buffer_type = MYSQL_TYPE_BLOB;
            const auto view = jh::pod::bytes_view::from(user.friends);
            auto blob_len = static_cast<unsigned long>(view.len);

            bind[3].buffer_type = MYSQL_TYPE_BLOB;
            bind[3].buffer = const_cast<std::byte *>(view.data);
            bind[3].buffer_length = blob_len;
            bind[3].length = &blob_len;


            if (mysql_stmt_bind_param(stmt, bind) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            if (mysql_stmt_execute(stmt) != 0)
                throw std::runtime_error(mysql_stmt_error(stmt));

            mysql_stmt_close(stmt);
        }
    };
}
