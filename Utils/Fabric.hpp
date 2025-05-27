#pragma once

#include <jh/pod>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <random>

extern std::mt19937 &global_rng();

namespace fabric {
    static constexpr jh::pod::array<const char *, 64> male_names =
            {{
                     "Liam", "Noah", "Oliver", "Elijah", "James",
                     "William", "Benjamin", "Lucas",
                     "Henry", "Alexander", "Mason", "Michael",
                     "Ethan", "Daniel", "Jacob", "Logan",
                     "Jackson", "Levi", "Sebastian", "Mateo",
                     "Jack", "Owen", "Theodore", "Aiden",
                     "Samuel", "Joseph", "John", "David",
                     "Wyatt", "Matthew", "Luke", "Asher",
                     "Carter", "Julian", "Grayson", "Leo",
                     "Jayden", "Gabriel", "Isaac", "Lincoln",
                     "Anthony", "Hudson", "Dylan", "Ezra",
                     "Thomas", "Charles", "Christopher", "Jaxon",
                     "Maverick", "Josiah", "Isaiah", "Andrew",
                     "Elias", "Joshua", "Nathan", "Caleb",
                     "Ryan", "Adrian", "Miles", "Eli", "Nolan",
                     "Christian", "Aaron", "Cameron"
             }};

    static constexpr jh::pod::array<const char *, 64> female_names =
            {{
                     "Olivia", "Emma", "Charlotte", "Amelia",
                     "Sophia", "Isabella", "Ava", "Mia",
                     "Evelyn", "Luna", "Harper", "Camila",
                     "Gianna", "Elizabeth", "Eleanor", "Ella",
                     "Abigail", "Emily", "Sofia", "Avery",
                     "Scarlett", "Grace", "Chloe", "Victoria",
                     "Riley", "Aria", "Lily", "Aurora", "Nora",
                     "Hazel", "Zoey", "Stella",
                     "Hannah", "Addison", "Leah", "Lucy",
                     "Eliana", "Ivy", "Willow", "Emilia",
                     "Layla", "Penelope", "Ellie", "Madison",
                     "Skylar", "Nova", "Paisley", "Savannah",
                     "Brooklyn", "Claire", "Bella", "Aubrey",
                     "Audrey", "Violet", "Kinsley", "Genesis",
                     "Maya", "Naomi", "Aaliyah", "Eliza",
                     "Alice", "Ariana", "Autumn", "Eva"
             }};

    static constexpr jh::pod::array<const char *, 128> surnames =
            {{
                     "Smith", "Johnson", "Williams", "Brown",
                     "Jones", "Garcia", "Miller", "Davis",
                     "Rodriguez", "Martinez", "Hernandez",
                     "Lopez", "Gonzalez", "Wilson", "Anderson",
                     "Thomas",
                     "Taylor", "Moore", "Jackson", "Martin",
                     "Lee", "Perez", "Thompson", "White",
                     "Harris", "Sanchez", "Clark", "Ramirez",
                     "Lewis", "Robinson", "Walker", "Young",
                     "Allen", "King", "Wright", "Scott", "Torres",
                     "Nguyen", "Hill", "Flores",
                     "Green", "Adams", "Nelson", "Baker", "Hall",
                     "Rivera", "Campbell", "Mitchell",
                     "Carter", "Roberts", "Gomez", "Phillips",
                     "Evans", "Turner", "Diaz", "Parker",
                     "Cruz", "Edwards", "Collins", "Reyes",
                     "Stewart", "Morris", "Morales", "Murphy",
                     "Cook", "Rogers", "Gutierrez", "Ortiz",
                     "Morgan", "Cooper", "Peterson", "Bailey",
                     "Reed", "Kelly", "Howard", "Ramos", "Kim",
                     "Cox", "Ward", "Richardson",
                     "Watson", "Brooks", "Chavez", "Wood",
                     "James", "Bennett", "Gray", "Mendoza",
                     "Ruiz", "Hughes", "Price", "Alvarez",
                     "Castillo", "Sanders", "Patel", "Myers",
                     "Long", "Ross", "Foster", "Jimenez",
                     "Powell", "Jenkins", "Perry", "Russell",
                     "Sullivan", "Bell", "Coleman", "Butler",
                     "Henderson", "Barnes", "Gonzales", "Fisher",
                     "Vasquez", "Simmons", "Romero", "Jordan",
                     "Patterson", "Alexander", "Hamilton",
                     "Graham",
                     "Reynolds", "Griffin", "Wallace", "Moreno",
                     "West", "Cole", "Hayes", "Bryant"
             }};
    static constexpr jh::pod::array<const char *, 32> avatar_urls =
            {
#include "avatars"
            };

    static constexpr jh::pod::array<const char *, 16> interests =
            {{
                     "Sports", "Music", "Movies", "Books",
                     "Travel", "Food", "Gaming", "Art",
                     "Technology", "Fitness", "Fashion",
                     "Nature", "Photography", "Cooking",
                     "DIY", "Gardening"
             }};

    static constexpr jh::pod::array<const char *, 16> one_hot_tags =
            {{"Age:Teenager", "Age:Young Adult", "Age:Adult", "Age:Senior",

              "Education:High School", "Education:Bachelor", "Education:Master", "Education:PhD",

              "Econ:Blue Collar", "Econ:White Collar", "Econ:Creative Worker", "Econ:Entrepreneur",

              "Stage:Student", "Stage:Working Adult", "Stage:Homemaker", "Stage:Retired"}};

    static constexpr jh::pod::array<jh::pod::pair<const char *, const char *>, 48> boolean_tags =
            {{
                     /// << Pilosohpical tags >>
                     {"Spiritual", "Pragmatic"},
                     {"Idealistic", "Realistic"},
                     {"Analytical", "Empathetic"},
                     {"Optimistic", "Pessimistic"},
                     {"Adventurous", "Cautious"},
                     {"Social Butterfly", "Loner"},
                     {"Extroverted", "Introverted"},
                     {"Independent", nullptr},
                     {"Ambitious", nullptr},
                     {"Open-minded", nullptr},
                     {"Philosophical", nullptr},
                     {"Creative", nullptr},
                     {"Logical", nullptr},
                     {"Practical", nullptr},

                     /// << social roles >>
                     {"Parent", nullptr},
                     {"Pet Owner", nullptr},
                     {"Is Creator", nullptr},

                     /// << Lifestyle tags >>
                     {"Early Riser", "Night Owl"},
                     {"Homebody", "Traveler"},
                     {"Fitness Buff", "Couch Potato"},
                     {"Volunteer", nullptr},
                     {"Fashionista", nullptr},
                     {"Health Conscious", nullptr},

                     /// << geeky tags >>
                     {"Tech-savvy", "Tech-averse"},
                     {"DIY Enthusiast", nullptr},
                     {"Python Lover", nullptr},
                     {"Science Nerd", nullptr},
                     {"History Buff", nullptr},
                     {"Language Learner", nullptr},
                     {"Puzzle Solver", nullptr},
                     {"Board Game Enthusiast", nullptr},

                     /// << Hobbies and interests >>
                     {"Photographer", nullptr},
                     {"Gourmet", nullptr},
                     {"Bookworm", nullptr},
                     {"Gamer", nullptr},
                     {"Music Lover", nullptr},
                     {"Movie Buff", nullptr},
                     {"Enjoys Biking", nullptr},
                     {"Enjoys Gardening", nullptr},
                     {"Enjoys Writing", nullptr},
                     {"Enjoys Board Games", nullptr},
                     {"Caring", nullptr},
                     {"Traveler", nullptr},
                     {"Hobbyist", nullptr},
                     {"Collector", nullptr},


                     /// << environmental and dietary tags >>
                     {"Eco-conscious", nullptr},
                     {"Environmentalist", nullptr},
                     {"Vegetarian/Vegan", nullptr},
             }};

    struct UserProfile {
        std::string first_name;
        std::string surname;
        bool gender{}; // 0 - male, 1 - female
        std::string avatar_url;
        std::vector<std::pair<std::string, int8_t>> interests; // up to 16 interests
        std::vector<std::string> tags; // mixed one-hot and boolean tags
    };


    struct UsersFabric {
        uint64_t user_id;
        uint32_t first_name_id;
        uint32_t last_name_id;
        uint32_t avatar_id;
    };

    struct UserSimpleProfile {
        std::string first_name;
        std::string surname;
        bool gender{}; // 0 - male, 1 - female
        std::string avatar_url;
    };


    std::vector<std::pair<std::string, int8_t>> decode_interests(uint64_t bits) {
        std::vector<std::pair<std::string, int8_t>> result;
        result.reserve(16);
        for (const jh::pod::pod_like auto &interest: interests) {
            result.emplace_back(interest, bits & 0xF);
            bits >>= 4;
        }
        return result;
    }

    std::vector<std::string> decode_tags(uint64_t bits) {
        std::vector<std::string> result;
        result.reserve(4 + 48); // 4 possible one-hot tags + 48 boolean tags

        for (const char *tag: fabric::one_hot_tags) {
            if (bits & 1) {
                result.emplace_back(tag);
            }
            bits >>= 1;
        }

        for (const jh::pod::pod_like auto &tag_pair: fabric::boolean_tags) {
            bool bit = bits & 1;
            if (tag_pair.second == nullptr) {
                if (bit) result.emplace_back(tag_pair.first);
            } else {
                result.emplace_back(bit ? tag_pair.first : tag_pair.second);
            }
            bits >>= 1;
        }

        return result;
    }

    uint64_t generate_random_interests() {
        auto &gen = global_rng();

        std::uniform_int_distribution<> strong_dist(5, 15);
        std::uniform_int_distribution<> weak_dist(0, 7);
        std::uniform_int_distribution<> strong_count_dist(3, 5);

        std::vector<int> indices(16);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), gen);

        size_t strong_count = strong_count_dist(gen);
        uint64_t bits = 0;

        for (size_t i = 0; i < 16; ++i) {
            uint8_t val = (i < strong_count)
                          ? strong_dist(gen)
                          : weak_dist(gen);
            bits |= (uint64_t(val & 0xF) << (indices[i] * 4));
        }

        return bits;
    }


    uint64_t generate_random_tags() {
        auto &gen = global_rng();

        uint64_t bits = 0;

        // One-hot: 4 groups of 4 tags (first 16 bits)
        for (int group = 0; group < 4; ++group) {
            int base = group * 4;
            int pick = std::uniform_int_distribution<>(0, 3)(gen);
            bits |= (1ULL << (base + pick));
        }

        // Boolean tags (remaining 48 bits)
        uint64_t boolean_part = std::uniform_int_distribution<uint64_t>(
                0, (1ULL << 48) - 1)(gen);
        bits |= (boolean_part << 16);

        return bits;
    }


    UserProfile make_user_profile(
            uint32_t first_name_id,
            uint32_t last_name_id,
            uint32_t avatar_id,
            uint64_t interests_16,
            uint64_t base_64_bits
    ) noexcept {
        UserProfile user;

        user.gender = avatar_id % 2;
        user.first_name = user.gender == 0
                          ? male_names[first_name_id % jh::pod::array<const char *, 64>::size()]
                          : female_names[first_name_id % jh::pod::array<const char *, 64>::size()];
        user.surname = surnames[last_name_id % jh::pod::array<const char *, 128>::size()];
        user.avatar_url = avatar_urls[avatar_id % jh::pod::array<const char *, 32>::size()];
        user.interests = decode_interests(interests_16);
        user.tags = decode_tags(base_64_bits);

        return user;
    }

    UserSimpleProfile make_user_simple_profile(
            UsersFabric fabric_entry
    ) noexcept {
        UserSimpleProfile user;
        user.gender = fabric_entry.avatar_id % 2;
        user.first_name = user.gender == 0
                          ? male_names[fabric_entry.first_name_id % jh::pod::array<const char *, 64>::size()]
                          : female_names[fabric_entry.first_name_id % jh::pod::array<const char *, 64>::size()];
        user.surname = surnames[fabric_entry.last_name_id % jh::pod::array<const char *, 128>::size()];
        user.avatar_url = avatar_urls[fabric_entry.avatar_id % jh::pod::array<const char *, 32>::size()];

        return user;
    }
} // namespace fabric