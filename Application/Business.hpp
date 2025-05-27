#pragma once

#include <set>
#include <unordered_set>
#include <queue>
#include <cstdint>
#include <jh/pod>
#include <random>
#include <boost/json.hpp>

#include "../Entities/UserModel.hpp"
#include "UserModelHandler.hpp"
#include "FabricInfoHandler.hpp"
#include "../Entities/UserModel.hpp"
#include "../Utils/Fabric.hpp"


namespace social {

    JH_POD_STRUCT(InteractionEntry,
                  uint32_t user1_id;
                          uint32_t user2_id;
                          uint32_t score;
    );

    constexpr uint8_t LOW_THRESHOLD = 48;
    constexpr uint8_t HIGH_THRESHOLD = 56;

    using interaction_input = std::vector<InteractionEntry>;

    inline uint8_t interest_cost(const uint8_t score) {
        // highest as 128, higher score -> lower A* cost
        if (score >= 112) return 1;
        if (score >= 96) return 2;
        if (score >= 80) return 3;
        return 4;
    }

    inline pod::array<pod::pair<uint32_t, uint8_t>, 256>
    _score_all_friends(const UserModel &self, const UserModel &node,
                       const std::unordered_map<uint32_t, social::UserProfileView> &profile_map) {
        struct FriendInfo {
            uint32_t id;
            uint32_t score;
        };

        FriendInfo temp[256];
        size_t count = 0;

        for (const auto &[fid, inter]: node.friends) {
            if (fid != INVALID_FRIEND_ID) {
                temp[count++] = {fid, inter};
            }
        }

        std::sort(temp, temp + count, [](const FriendInfo &a, const FriendInfo &b) {
            return b.score < a.score;
        });

        pod::array<pod::pair<uint32_t, uint8_t>, 256> result{};

        for (size_t i = 0; i < count; ++i) {
            const uint32_t fid = temp[i].id;

            if (fid == self.user_id || !profile_map.count(fid)) continue;

            const auto &[_, interests_16, base_64_bits] = profile_map.at(fid);
            const uint8_t basics = match_basics(self.base_64_bits, base_64_bits);

            uint8_t cost = 4;

            if (i < 5) {
                if (basics >= HIGH_THRESHOLD)
                    cost = 1;
                else if (basics < LOW_THRESHOLD)
                    cost = 4;
                else
                    cost = interest_cost(match_interests(self.interests_16, interests_16));
            } else if (i < 20) {
                cost = (basics >= HIGH_THRESHOLD) ? 3 : 4;
            }

            result[i] = {fid, cost};
        }

        return result;
    }


    /// Including friends -> front page or strangers (friends of friends) -> people you might know
    template<size_t N>
    pod::array<uint32_t, N>
    recommend_A_star(const UserModel& self, const UserModelHandler& ctrl, uint8_t max_depth = 4) {
        struct Node {
            uint32_t user_id;
            uint8_t cost;
            uint8_t depth;

            bool operator>(const Node& other) const {
                if (cost != other.cost) return cost > other.cost;
                return user_id > other.user_id;
            }
        };

        std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
        std::unordered_map<uint32_t, uint8_t> best_cost;
        std::unordered_set<uint32_t> recommended;
        std::unordered_map<uint32_t, social::UserProfileView> profile_map;

        pod::array<uint32_t, N> result{};
        size_t filled = 0;

        open.push({self.user_id, 0, 0});
        best_cost[self.user_id] = 0;

        while (!open.empty() && filled < N) {
            Node current = open.top();
            open.pop();

            // Already recommended or self
            if (current.user_id != self.user_id && !recommended.contains(current.user_id)) {
                if (!social::is_friend(self, current.user_id)) {
                    result[filled++] = current.user_id;
                }
                recommended.insert(current.user_id);
            }

            if (current.depth >= max_depth)
                continue;

            const UserModel node = ctrl.load_user_by_id(current.user_id);

            // Lazy-load friends' profiles
            for (const auto& [fid, _] : node.friends) {
                if (fid == INVALID_FRIEND_ID) continue;
                if (!profile_map.contains(fid)) {
                    profile_map[fid] = ctrl.get_user_profile_view(fid);
                }
            }

            for (const auto& [fid, interaction_score] : node.friends) {
                if (fid == INVALID_FRIEND_ID || fid == self.user_id)
                    continue;

                // Lazy-load if still missing
                if (!profile_map.contains(fid)) {
                    profile_map[fid] = ctrl.get_user_profile_view(fid);
                }

                const auto& prof = profile_map.at(fid);
                const uint8_t basic_score = match_basics(self.base_64_bits, prof.base_64_bits);

                uint8_t cost = 4; // default
                if (basic_score >= HIGH_THRESHOLD) {
                    cost = 1;
                } else if (basic_score < LOW_THRESHOLD) {
                    cost = 4;
                } else {
                    cost = interest_cost(match_interests(self.interests_16, prof.interests_16));
                }

                uint8_t new_cost = current.cost + cost;
                if (!best_cost.contains(fid) || new_cost < best_cost[fid]) {
                    best_cost[fid] = new_cost;
                    open.push({fid, new_cost, static_cast<uint8_t>(current.depth + 1)});
                }
            }
        }

        return result;
    }

    template<size_t N>
    pod::array<uint32_t, N> recommend_strangers(const UserModel &self, const UserModelHandler &ctrl) {
        using Candidate = std::pair<uint32_t, uint8_t>; // <user_id, match_basics_score>
        std::vector<Candidate> candidates;

        for (auto interest_users = ctrl.build_interest_similarity<N * 2>(self); uint32_t id: interest_users) {
            if (id == INVALID_FRIEND_ID || find_friend_index(self, id) != INVALID_INDEX || id == self.user_id) continue;
            const auto [id_, interests_16, base_64_bits] = ctrl.get_user_profile_view(id);
            uint8_t score = match_basics(self.base_64_bits, base_64_bits);
            candidates.emplace_back(id_, score);
        }

        // Similar forward
        std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
            return b.second < a.second; // higher match_basics score first
        });

        pod::array<uint32_t, N> result{};
        size_t filled = 0;
        std::unordered_set<uint32_t> seen;

        for (const auto &[id, _]: candidates) {
            if (filled >= N) break;
            if (seen.insert(id).second) {
                result[filled++] = id;
            }
        }

        // Not fitting n, give several fill backs
        if (filled < N) {
            for (auto loose_users = ctrl.build_interest_similarity<N * 2>(self, 5, 2); uint32_t id: loose_users) {
                if (id == INVALID_FRIEND_ID || id == self.user_id || seen.count(id)) continue;
                const auto [id_, interests_16, base_64_bits] = ctrl.get_user_profile_view(id);
                uint8_t score = match_basics(self.base_64_bits, base_64_bits);
                candidates.emplace_back(id_, score);
            }
            // If having more users, can raise the mid_threshold to scan fewer users in the future.

            std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
                return b.second < a.second;
            });

            for (const auto &[id, _]: candidates) {
                if (filled >= N) break;
                if (seen.insert(id).second) {
                    result[filled++] = id;
                }
            }
        }

        return result;
    }

    inline uint32_t _batch_update_interactions(UserModelHandler &ctrl, const interaction_input &interactions) {
        if (interactions.empty()) return 0;

        uint32_t new_friends = 0;

        // Load all user_ids into memory first
        std::unordered_set<uint32_t> all_ids;
        for (const auto &[u1, u2, _]: interactions) {
            all_ids.insert(u1);
            all_ids.insert(u2);
        }

        std::unordered_map<uint32_t, UserModel> user_map;
        for (uint32_t id: all_ids) {
            user_map.emplace(id, ctrl.load_user_by_id(id));
        }

        // Process all interactions
        for (const auto &[u1, u2, score]: interactions) {
            auto &user1 = user_map[u1];
            auto &user2 = user_map[u2];

            const bool already_friends = social::find_friend_index(user1, u2) != INVALID_INDEX;

            if (already_friends) {
                social::add_interaction(user1, u2, score);
                social::add_interaction(user2, u1, score);
            } else {
                const bool can_add1 = social::find_insertable_friend_slot(user1) != INVALID_INDEX;
                const bool can_add2 = social::find_insertable_friend_slot(user2) != INVALID_INDEX;

                if (can_add1 && can_add2) [[likely]] {
                    social::add_friend(user1, u2, score);
                    social::add_friend(user2, u1, score);
                    ++new_friends;
                } // else: skip, no room for friendship (unlikely case)
            }
        }

        // Save in batches of 256
        std::vector<UserModel> buffer;
        buffer.reserve(256);

        for (auto &[id, user]: user_map) {

            /// For simplicity, decay interactions before saving
            social::decay_interactions(user);
            buffer.emplace_back(user);

            if (buffer.size() == 256) {
                ctrl.batch_insert_users(buffer);
                buffer.clear();
            }
        }

        auto batch_map = ctrl.batch_load_users_by_ids(all_ids);
        user_map.insert(batch_map.begin(), batch_map.end());

        return new_friends;
    }
}

namespace fabric::api {
    JH_POD_STRUCT(DayResult,
                  uint32_t new_users;
                          uint32_t new_friendships;
                          uint32_t total_interactions;
    );

    inline void clear_all(social::UserModelHandler &user_handler, FabricInfoHandler &fabric_handler) {
        user_handler.clear_user_table();
        fabric_handler.clear_all();
    }

    inline uint32_t _generate_users(uint32_t
                                   start_id,
                                   uint32_t count, social::UserModelHandler
                                   &user_handler,
                                   FabricInfoHandler &fabric_handler
    ) {
        std::vector<UsersFabric> fabric_entries;
        std::vector<social::UserModel> user_models;

        std::uniform_int_distribution<uint32_t> fname_dist(0, 63);
        std::uniform_int_distribution<uint32_t> lname_dist(0, 127);
        std::uniform_int_distribution<uint32_t> avatar_dist(0, 31);
        auto &rng = global_rng();

        for (
                uint64_t i = 0;
                i < count;
                ++i) {
            uint32_t id = start_id + i;
            uint32_t first_name_id = fname_dist(rng);
            uint32_t last_name_id = lname_dist(rng);
            uint32_t avatar_id = avatar_dist(rng);
            uint64_t interests = generate_random_interests();
            uint64_t tags = generate_random_tags();

            fabric_entries.
                    emplace_back(id, first_name_id, last_name_id, avatar_id
            );
            user_models.
                    emplace_back(id, interests, tags
            );
        }

        std::uniform_int_distribution<size_t> pick_dist(0, user_models.size() - 1);

        for (
            auto &user
                : user_models) {
            for (
                    int attempt = 0;
                    attempt < 10; ++attempt) {
                size_t j = pick_dist(rng);
                auto &other = user_models[j];
                if (user.user_id == other.user_id) continue;

                if (
                        social::find_friend_index(user, other
                                .user_id) != INVALID_INDEX) {
                    social::add_interaction(user, other
                            .user_id, 5);
                    social::add_interaction(other, user
                            .user_id, 5);
                } else if (
                        social::add_friend_mutual(user, other
                        )) {
                    social::add_interaction(user, other
                            .user_id);
                    social::add_interaction(other, user
                            .user_id);
                }
            }
        }

        fabric_handler.
                batch_insert_users(fabric_entries);
        user_handler.
                batch_insert_users(user_models);
        return
                count;
    }

    inline uint32_t initialize_population(social::UserModelHandler &user_handler,
                                          FabricInfoHandler &fabric_handler) {
        std::uniform_int_distribution<uint32_t> dist(64, 128);
        uint32_t count = dist(global_rng());
        _generate_users(1, count, user_handler, fabric_handler);
        return count;
    }


    DayResult next_day(social::UserModelHandler &user_handler,
                       FabricInfoHandler &fabric_handler) {

        const uint32_t total = fabric_handler.get_count();
        if (total < 3) initialize_population(user_handler, fabric_handler);

        std::uniform_int_distribution<uint32_t> total_interact_dist(
                std::max(2048u, total * 2), std::max(4096u, total * 3));
        uint32_t total_interactions = total_interact_dist(global_rng());

        social::interaction_input interactions;
        interactions.reserve(total_interactions);

        std::uniform_int_distribution<uint32_t> id_dist(1, total);
        std::uniform_int_distribution<uint32_t> score_dist(1, 30);
        auto &rng = global_rng();

        for (uint32_t i = 0; i < total_interactions; ++i) {
            uint32_t u1 = id_dist(rng);
            uint32_t u2 = id_dist(rng);
            if (u1 == u2) continue;
            uint32_t score = score_dist(rng);
            interactions.emplace_back(u1, u2, score);
        }

        uint32_t new_friendships = social::_batch_update_interactions(user_handler, interactions);

        std::uniform_int_distribution<uint32_t> new_user_dist(
                std::min(256u, total / 20), std::max(1024u, total / 20));
        uint32_t new_user_count = new_user_dist(global_rng());

        _generate_users(total + 1, new_user_count, user_handler, fabric_handler);

        return DayResult{
                .new_users = new_user_count,
                .new_friendships = new_friendships,
                .total_interactions = static_cast<uint32_t>(interactions.size())
        };
    }

    fabric::UserProfile
    get_user_profile(uint32_t id, social::UserModelHandler &user_handler, FabricInfoHandler &fabric_handler) {
        const auto model = user_handler.get_user_profile_view(id);
        const auto fabric = fabric_handler.load_user(id);
        return make_user_profile(fabric.first_name_id, fabric.last_name_id, fabric.avatar_id,
                                 model.interests_16, model.base_64_bits);
    }

    fabric::UsersFabric
    get_user_simple_profile(uint32_t id, FabricInfoHandler &fabric_handler) {
        if (id == 0 || id > fabric_handler.get_count()) [[unlikely]] {
            throw std::out_of_range("Invalid user ID");
        }
        return fabric_handler.load_user(id);
    }

    std::vector<uint32_t>
    get_user_friends(uint32_t id, social::UserModelHandler &user_handler, FabricInfoHandler &fabric_handler) noexcept {
        if (id == 0 || id > fabric_handler.get_count()) [[unlikely]] return {}; // Invalid user ID
        const auto usr = user_handler.load_user_by_id(id);
        std::vector<uint32_t> friends;
        friends.reserve(social::friend_list::size());

        for (const auto &[fid, _]: usr.friends) {
            if (fid != INVALID_FRIEND_ID) {
                friends.emplace_back(fid);
            }
        }
        return friends;
    }

    boost::json::value to_json(const UserProfile &user, uint32_t id) noexcept {
        boost::json::object obj;

        obj["user_id"] = id;
        obj["first_name"] = user.first_name;
        obj["surname"] = user.surname;
        obj["gender"] = user.gender;
        obj["avatar_url"] = user.avatar_url;

        boost::json::array interests_arr;
        for (const auto &[name, value]: user.interests) {
            boost::json::object item;
            item["name"] = name;
            item["value"] = value;
            interests_arr.emplace_back(item);
        }
        obj["interests"] = interests_arr;

        boost::json::array tags_arr;
        for (const auto &tag: user.tags) {
            tags_arr.emplace_back(tag);
        }
        obj["tags"] = tags_arr;

        return obj;
    }

    boost::json::value simple_json(const UsersFabric &user, uint32_t id) noexcept {
        boost::json::object obj;

        const auto simple_user = fabric::make_user_simple_profile(user);

        obj["user_id"] = id;
        obj["first_name"] = simple_user.first_name;
        obj["surname"] = simple_user.surname;
        obj["gender"] = simple_user.gender;
        obj["avatar_url"] = simple_user.avatar_url;

        return obj;
    }

    inline uint32_t random_user_id(FabricInfoHandler &fabric_handler) noexcept {
        const auto total_users = fabric_handler.get_count();
        if (total_users == 0) [[unlikely]] return 0; // Invalid case, should not happen in practice
        auto &rng = global_rng();
        std::uniform_int_distribution<uint32_t> dist(1, fabric_handler.get_count());
        return dist(rng);
    }

}