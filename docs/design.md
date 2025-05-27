# 🧩 Front-End Requirements Design Document

**Project Title**: Synthetic User Simulation Interface
**Inspiration Style**: *While True: learn()* (Game-like simulation)

---

## 🎯 Goal

Create an interactive simulation dashboard that **mimics synthetic social behavior**, visualizing **daily changes**, *
*user profiles**, and **recommendation networks** in a game-like interface inspired by *While True: learn()*.

---

## 🧰 Core Features (Refined)

### 1. **Initialization & Time Simulation**

* **Day Tracking**:

    * Initialize at **Day 0**
    * Track current day number (`Day N`) internally

* **Reset Simulation**

    * Button: *"Reset Simulation"*
    * Calls `/api/refresh_db`
    * Resets user pool and sets simulation to Day 0
    * Reload total user count using `/api/get_total_count`

* **Advance Simulation (Run Next Day)**

    * Button: *"Next Day"*
    * Calls `/api/simulate_day`
    * Updates:

        * **New users** (Δ in population)
        * **New friendships**
        * **Daily total interactions**
    * Show these as **per-day diffs**, not cumulative

        * Example:

          ```
          Day 3
          +18 users, +274 friendships, 3,921 interactions today
          ```

* **Daily Activity History**

    * Chart: Bar graph or line chart of `total_interactions` per day
    * Tooltip: "Today’s Activity Volume"

---

### 2. **User Profile Viewer**

* **User Selector**

    * Input box + Random button
    * Valid user ID: \[1, `total_users`]
    * Calls `/api/get_user_profile?id={id}` to fetch and show full profile

* **Profile Display**

    * Fields:

        * `first_name` + `surname`
        * `gender` (♂️ / ♀️ badge)
        * `avatar` (from base64)
        * `interests`: visualize as **percent bars**

            * 0 → 0%
            * 15 → 100%
        * `tags`: boolean flags as visual badges or toggles

---

### 3. **Social Visualization & Recommendations**

#### 3.1 Friends

* `/api/get_user_friends?id={id}` → list of user IDs
* Fetch details via `/api/batch_get_simple_profiles`
* Display: avatars + names (grid or horizontal scroll)

#### 3.2 FOF Recommendations

* `/api/recommend_fof?id={id}`
* Also uses `batch_get_simple_profiles`
* Label: **"Friend-of-Friend Suggestions"**
* Description: A\* search, **max depth 4**, excludes self and direct friends

#### 3.3 Stranger Match Recommendations

* `/api/recommend_strangers?id={id}`
* Based on **interest/tag similarity**
* Also uses `batch_get_simple_profiles`
* Label: **"You Might Like"** or **"Similar Strangers"**

---

## 🎨 UI/UX Additions

* 💬 **Interaction Logs**:

    * Rolling history of daily events (optional)
    * “+X users joined”, “Y interactions occurred today”

* 📊 **Interest Bars**:

    * Convert interest score \[0–15] → percentage (0%–100%)
    * Label each bar by interest name (if available)

* 🎨 **Tags**:

    * Display 64-bit tags as small colorful badges
    * Tooltips for known tag names (optional)

* 🧠 **AI Feel**:

    * Add "processing" animations during recommendation calls
    * Visualize FOF suggestions like a map/tree if possible

---

## 📌 API Usage Summary (Contextual)

| Feature             | Endpoint                         | Notes                          |
|---------------------|----------------------------------|--------------------------------|
| Init/reset          | `/api/refresh_db`                | Reset to Day 0                 |
| Total user count    | `/api/get_total_count`           | After sim/day or reset         |
| Daily sim           | `/api/simulate_day`              | For Δmetrics and daily traffic |
| Random user         | `/api/random_user_id`            | For exploration                |
| Full profile        | `/api/get_user_profile`          | Includes interests, tags       |
| Friends             | `/api/get_user_friends`          | Returns friend ID list         |
| FOF recommendations | `/api/recommend_fof`             | Based on A\* search            |
| Stranger match      | `/api/recommend_strangers`       | Based on model similarity      |
| Batch profile load  | `/api/batch_get_simple_profiles` | Fetch multiple user cards      |
