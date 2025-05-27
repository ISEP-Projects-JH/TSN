#pragma once
#include <cstdint>
#include <jh/pod>
#include <algorithm> // for std::sort
#include <cstring>   // for std::memcpy
#include <sstream>   // NOLINT for std::ostream

constexpr uint32_t INVALID_FRIEND_ID = 0;
constexpr uint16_t INVALID_INDEX = static_cast<uint16_t>(-1);

namespace pod = jh::pod;

namespace social{
    /// <friend_id, interaction_score>
    using friend_list = pod::array<pod::pair<uint32_t, uint32_t>, 256>;

    struct UserModel final{
        uint32_t user_id;
        uint64_t interests_16;
        uint64_t base_64_bits;
        friend_list friends;  /// not followers but regularly interacted people
    };

    constexpr uint8_t match_interests(uint64_t a, uint64_t b) {
        uint8_t res = 0;
        for (uint8_t i = 0; i < 16; ++i) {
            const uint8_t a_lvl = a & 0xF;
            const uint8_t b_lvl = b & 0xF;
            const uint8_t diff = a_lvl > b_lvl ? a_lvl - b_lvl : b_lvl - a_lvl;
            res += 0b1000 >> diff;
            a >>= 4;
            b >>= 4;
        }
        return res;
    }

    constexpr uint8_t match_basics(uint64_t a, uint64_t b) {
#if defined(__GNUC__) || defined(__clang__)
        return 64 - __builtin_popcountll(a ^ b);
#else
        // Fallback popcount if needed
        uint64_t x = a ^ b;
        x = x - ((x >> 1) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
        return static_cast<uint8_t>((x * 0x0101010101010101ULL) >> 56);
#endif
    }

    inline void add_interaction(UserModel& self, const uint32_t friend_id, const uint32_t amount = 1) {
        for (auto&[first, second] : self.friends) {
            if (first == friend_id || first == INVALID_FRIEND_ID) {
                first = friend_id;
                second += amount;
                return;
            }
        }
    }

    template<size_t N = 5>
    pod::array<uint32_t, N> top_friends(const UserModel& self) {
        struct Scored {
            uint32_t id;
            uint32_t score;
        };

        Scored scores[256];
        size_t count = 0;
        for (const auto&[first, second] : self.friends) {
            if (first != INVALID_FRIEND_ID) {
                scores[count++] = {first, second};
            }
        }

        std::sort(scores, scores + count, [](const Scored& a, const Scored& b) {
            return b.score < a.score;
        });

        pod::array<uint32_t, N> result{};
        for (size_t i = 0; i < std::min(N, count); ++i) {
            result[i] = scores[i].id;
        }
        return result;
    }

    inline void decay_interactions(UserModel& self, const float rate = 0.95f) {
        for (auto&[first, second] : self.friends) {
            second = static_cast<uint32_t>(static_cast<double>(second) * rate);
            if (second == 0){
                first = INVALID_FRIEND_ID;
            }
        }
    }

    constexpr uint16_t find_friend_index(const UserModel& user, const uint32_t friend_id) {
        for (uint16_t i = 0; i < static_cast<uint16_t>(friend_list::size()); ++i) {
            if (user.friends[i].first == friend_id) return i;
        }
        return INVALID_INDEX;
    }

    inline uint16_t find_insertable_friend_slot(const UserModel& user) {
        for (uint16_t i = 0; i < static_cast<uint16_t>(friend_list::size()); ++i) {
            if (user.friends[i].first == INVALID_FRIEND_ID) return i;
        }
        return INVALID_INDEX;
    }

    inline bool add_friend(UserModel& user, const uint32_t friend_id, const uint32_t score = 10) {
        if (find_friend_index(user, friend_id) != INVALID_INDEX)
            return true; // already exists

        const uint16_t idx = find_insertable_friend_slot(user);
        if (idx == INVALID_INDEX) return false;

        user.friends[idx].first = friend_id;
        user.friends[idx].second = score;
        return true;
    }

    inline bool remove_friend(UserModel& user, const uint32_t friend_id) {
        const uint16_t idx = find_friend_index(user, friend_id);
        if (idx == INVALID_INDEX) return false;

        user.friends[idx].first = INVALID_FRIEND_ID;
        user.friends[idx].second = 0;
        return true;
    }

    inline bool add_friend_mutual(UserModel& a, UserModel& b) {
        const bool ok_a = add_friend(a, b.user_id);
        const bool ok_b = add_friend(b, a.user_id);
        return ok_a && ok_b;
    }

    inline bool remove_friend_mutual(UserModel& a, UserModel& b) {
        const bool ok_a = remove_friend(a, b.user_id);
        const bool ok_b = remove_friend(b, a.user_id);
        return ok_a && ok_b;
    }

    constexpr bool is_friend(const UserModel u, const uint32_t target_id) {
        return std::any_of(u.friends.begin(), u.friends.end(), [&](const auto &pair) {
            return pair.first == target_id;
        });
    }

    inline std::ostream& operator<<(std::ostream& os, const UserModel& u) {
        os << "UserModel ID: " << u.user_id << "\n";
        os << "Interests_16: 0x" << std::hex << u.interests_16 << std::dec << "\n";
        os << "Base_64_Bits: 0b";
        for (int i = 63; i >= 0; --i)
            os << ((u.base_64_bits >> i) & 1);
        os << "\nTop Friends:\n";

        for (auto top = top_friends<5>(u); const uint32_t fid : top) {
            if (fid != INVALID_FRIEND_ID)
                os << " - Friend ID: " << fid << "\n";
        }
        return os;
    }

}
