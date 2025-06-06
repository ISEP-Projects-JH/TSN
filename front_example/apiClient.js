const BASE_URL = 'http://localhost:8080/api';
const PING_URL = 'http://localhost:8080/ping';

// Connect to db
const dbConfig = {
    host: 'localhost',
    user: 'usr_mdl_usr',
    password: 'example',
    database: 'synthetic_users_local',
    port: 3306,
};

// 🌐 Common CORS Headers
async function request(url, options = {}) {
    const res = await fetch(url, {
        mode: 'cors',
        headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            ...(options.headers || {}),
        },
        ...options,
    });
    return res.json();
}

// private::
async function connectDB() {
    return request(`${BASE_URL}/set_db_connection`, {
        method: 'POST',
        body: JSON.stringify(dbConfig),
    });
}

// 🌐 Public APIs

export async function ping() {
    return request(PING_URL);
}


export async function refreshDB() {
    return request(`${BASE_URL}/refresh_db`, { method: 'POST' });
}

export async function getRandomUserID() {
    return request(`${BASE_URL}/random_user_id`);
}

export async function getTotalCount() {
    return request(`${BASE_URL}/get_total_count`);
}

export async function batchGetSimpleProfiles(userIds) {
    return request(`${BASE_URL}/batch_get_simple_profiles`, {
        method: 'POST',
        body: JSON.stringify(userIds),
    });
}

export async function getUserProfile(id) {
    return request(`${BASE_URL}/get_user_profile?id=${id}`);
}

export async function getUserProfileSimple(id) {
    return request(`${BASE_URL}/get_user_profile_simple?id=${id}`);
}

export async function getUserFriends(id) {
    return request(`${BASE_URL}/get_user_friends?id=${id}`);
}

export async function simulateDay() {
    return request(`${BASE_URL}/simulate_day`, { method: 'POST' });
}

export async function recommendFOF(id) {
    return request(`${BASE_URL}/recommend_fof?id=${id}`);
}

export async function recommendStrangers(id) {
    return request(`${BASE_URL}/recommend_strangers?id=${id}`);
}

// 🔄 INIT：ping + connectDB()
export async function initializeAPI() {
    try {
        const pingRes = await ping();
        if (!pingRes || pingRes.status !== 'alive') {
            return { status: false, message: 'Server is not alive' };
        }

        const connRes = await connectDB();
        if (connRes.status !== 'connected') {
            return { status: false, message: 'Failed to connect to DB' };
        }

        return { status: true };
    } catch (err) {
        return {
            status: false,
            message: err?.message || 'Unknown error during initialization',
        };
    }
}
