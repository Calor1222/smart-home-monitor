const CLOUD_FUNCTION_URL = "https://1347976579-cky20hoj3r.ap-guangzhou.tencentscf.com";
const CLOUD_CONTROL_URL = CLOUD_FUNCTION_URL + "/control";
console.log("app.js loaded");

const TEXT = {
    online: "\u5728\u7ebf",
    offline: "\u79bb\u7ebf",
    noData: "--",
    loadFailed: "\u6570\u636e\u83b7\u53d6\u5931\u8d25",
    pmLevelPrefix: "\u7a7a\u6c14\u8d28\u91cf\u7b49\u7ea7\uff1a",
    good: "\u4f18",
    fair: "\u826f",
    light: "\u8f7b\u5ea6\u6c61\u67d3",
    danger: "\u91cd\u5ea6\u6c61\u67d3"
};

const initialData = {
    deviceOnline: null,
    updatedAt: null,
    current: {
        Temp: null,
        Hum: null,
        Smoke: null,
        pm: null
    },
    history: []
};

const dashboardState = {
    deviceOnline: initialData.deviceOnline,
    updatedAt: initialData.updatedAt,
    current: { ...initialData.current },
    history: [...initialData.history]
};

const controlState = {
    fan: false,
    atomizer: false,
    motor: false
};
let globalMode = "auto";

const MAX_HISTORY_POINTS = 20;
let trendChart;

function getPmLevel(pm) {
    if (pm == null) {
        return TEXT.noData;
    }
    if (pm <= 35) {
        return TEXT.good;
    }
    if (pm <= 75) {
        return TEXT.fair;
    }
    if (pm <= 150) {
        return TEXT.light;
    }
    return TEXT.danger;
}

function formatTime(timeString) {
    if (!timeString) {
        return TEXT.noData;
    }

    const date = new Date(timeString);
    if (Number.isNaN(date.getTime())) {
        return timeString;
    }

    return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, "0")}-${String(date.getDate()).padStart(2, "0")} ${String(date.getHours()).padStart(2, "0")}:${String(date.getMinutes()).padStart(2, "0")}:${String(date.getSeconds()).padStart(2, "0")}`;
}

function formatChartTime(timeString) {
    if (!timeString) {
        return TEXT.noData;
    }

    const date = new Date(timeString);
    if (Number.isNaN(date.getTime())) {
        return timeString;
    }

    return `${String(date.getHours()).padStart(2, "0")}:${String(date.getMinutes()).padStart(2, "0")}:${String(date.getSeconds()).padStart(2, "0")}`;
}

function formatValue(value, suffix) {
    if (value == null) {
        return TEXT.noData;
    }
    return `${value.toFixed(1)} ${suffix}`;
}

function updateStatusBar(data) {
    const deviceStatus = document.getElementById("deviceStatus");
    const updateTime = document.getElementById("updateTime");

    if (data.deviceOnline == null) {
        deviceStatus.textContent = TEXT.noData;
    } else {
        deviceStatus.textContent = data.deviceOnline ? TEXT.online : TEXT.offline;
    }

    updateTime.textContent = formatTime(data.updatedAt);
}

function updateCards(data) {
    const { Temp, Hum, Smoke, pm } = data.current;

    document.getElementById("tempValue").textContent = formatValue(Temp, "\u00b0C");
    document.getElementById("humValue").textContent = formatValue(Hum, "%");
    document.getElementById("smokeValue").textContent = formatValue(Smoke, "ppm");
    document.getElementById("pmValue").textContent = formatValue(pm, "ug/m3");
    document.getElementById("pmLevel").textContent = pm == null ? TEXT.noData : `${TEXT.pmLevelPrefix}${getPmLevel(pm)}`;
}

function normalizeHistory(history) {
    return history.slice(-MAX_HISTORY_POINTS);
}

function createChartData(history) {
    const limitedHistory = normalizeHistory(history);

    return {
        labels: limitedHistory.map(item => formatChartTime(item.time)),
        datasets: [
            {
                label: "Temp",
                data: limitedHistory.map(item => item.Temp),
                borderColor: "#ff0000",
                backgroundColor: "rgba(255, 0, 0, 0.15)",
                borderWidth: 3,
                pointRadius: 3,
                pointHoverRadius: 5,
                tension: 0.3
            },
            {
                label: "Hum",
                data: limitedHistory.map(item => item.Hum),
                borderColor: "#0000ff",
                backgroundColor: "rgba(0, 0, 255, 0.15)",
                borderWidth: 3,
                pointRadius: 3,
                pointHoverRadius: 5,
                tension: 0.3
            },
            {
                label: "Smoke",
                data: limitedHistory.map(item => item.Smoke),
                borderColor: "#ffd700",
                backgroundColor: "rgba(255, 215, 0, 0.15)",
                borderWidth: 3,
                pointRadius: 3,
                pointHoverRadius: 5,
                tension: 0.3
            },
            {
                label: "pm",
                data: limitedHistory.map(item => item.pm),
                borderColor: "#00aa00",
                backgroundColor: "rgba(0, 170, 0, 0.15)",
                borderWidth: 3,
                pointRadius: 3,
                pointHoverRadius: 5,
                tension: 0.3
            }
        ]
    };
}

function initChart(history) {
    const context = document.getElementById("trendChart");
    const chartData = createChartData(history);

    if (trendChart) {
        trendChart.destroy();
    }

    trendChart = new Chart(context, {
        type: "line",
        data: chartData,
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: "top"
                }
            },
            scales: {
                x: {
                    ticks: {
                        maxTicksLimit: 6,
                        maxRotation: 0
                    }
                },
                y: {
                    beginAtZero: true
                }
            }
        }
    });
}

function updateChart(history) {
    if (!trendChart) {
        initChart(history);
        return;
    }

    const chartData = createChartData(history);
    trendChart.data.labels = chartData.labels;
    trendChart.data.datasets.forEach((dataset, index) => {
        dataset.data = chartData.datasets[index].data;
    });
    trendChart.update("none");
}

function updateUI(data) {
    updateStatusBar(data);
    updateCards(data);
    updateChart(data.history);
}

function appendHistoryPoint(data) {
    dashboardState.history.push({
        time: data.time || new Date().toLocaleString(),
        Temp: data.Temp ?? null,
        Hum: data.Hum ?? null,
        Smoke: data.Smoke ?? null,
        pm: data.pm ?? null
    });

    if (dashboardState.history.length > MAX_HISTORY_POINTS) {
        dashboardState.history.splice(0, dashboardState.history.length - MAX_HISTORY_POINTS);
    }
}

async function fetchOneNETData() {
    try {
        console.log("开始请求数据");
        console.log('璇锋眰URL:', CLOUD_FUNCTION_URL);
        console.log('璇锋眰浜戝嚱鏁?', CLOUD_FUNCTION_URL);

        const res = await fetch(CLOUD_FUNCTION_URL, {
            method: 'GET',
            mode: 'cors',
            cache: 'no-cache'
        });
        console.log('response:', res);
        if (!res.ok) {
            throw new Error(`HTTP ${res.status}`);
        }

        const data = await res.json();
        console.log('data:', data);
        console.log('杩斿洖鏁版嵁:', data);

        dashboardState.deviceOnline = true;
        dashboardState.updatedAt = data.time || new Date().toLocaleString();
        dashboardState.current = {
            Temp: Number(data.Temp),
            Hum: Number(data.Hum),
            Smoke: Number(data.Smoke),
            pm: Number(data.pm)
        };

        appendHistoryPoint({
            Temp: dashboardState.current.Temp,
            Hum: dashboardState.current.Hum,
            Smoke: dashboardState.current.Smoke,
            pm: dashboardState.current.pm,
            time: dashboardState.updatedAt
        });

        updateUI(dashboardState);
    } catch (err) {
        console.error('fetch error:', err);
        console.error('鑾峰彇鏁版嵁澶辫触', err);
        showError();
    }
}

function showError() {
    const deviceStatus = document.getElementById("deviceStatus");
    const updateTime = document.getElementById("updateTime");
    const pmLevel = document.getElementById("pmLevel");

    deviceStatus.textContent = TEXT.loadFailed;
    updateTime.textContent = TEXT.noData;
    pmLevel.textContent = TEXT.loadFailed;
}

async function sendControlCommand(params) {
    try {
        // 璋冪敤浜戝嚱鏁?control 鎺ュ彛
        console.log("POST control url:", CLOUD_CONTROL_URL);
        console.log("POST control body:", params);
        const res = await fetch(CLOUD_CONTROL_URL, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(params)
        });

        const data = await res.json();
        console.log("control result:", data);
        return data;
    } catch (err) {
        console.error("control error:", err);
        throw err;
    }
}

function getControlLabel(device, state) {
    if (device === "fan") {
        return state ? "\u98ce\u6247\uff1a\u5f00" : "\u98ce\u6247\uff1a\u5173";
    }
    if (device === "atomizer") {
        return state ? "\u96fe\u5316\u5668\uff1a\u5f00" : "\u96fe\u5316\u5668\uff1a\u5173";
    }
    return state ? "\u7535\u673a\uff1a\u5f00" : "\u7535\u673a\uff1a\u5173";
}

function updateControlButton(device) {
    const button = document.getElementById(`${device}Control`);
    const state = controlState[device];

    button.textContent = getControlLabel(device, state);
    button.classList.toggle("active", state);
}

function getGlobalModeLabel() {
    return globalMode === "manual" ? "\u624b\u52a8\u6a21\u5f0f" : "\u81ea\u52a8\u6a21\u5f0f";
}

function updateGlobalModeButton() {
    const button = document.getElementById("globalModeToggle");

    button.textContent = getGlobalModeLabel();
    button.classList.toggle("active", globalMode === "manual");
}

async function setGlobalMode(mode) {
    globalMode = mode;
    updateGlobalModeButton();
    await sendControlCommand({
        FanMode: mode,
        AtomizerMode: mode,
        MotorMode: mode
    });
}

function handleControlToggle(device) {
    controlState[device] = !controlState[device];
    globalMode = "manual";
    console.log("click control:", device, controlState[device]);
    updateGlobalModeButton();
    updateControlButton(device);
    if (device === "atomizer") {
        sendControlCommand({
            AtomizerMode: "manual",
            AtomizerSwitch: controlState[device]
        });
    } else if (device === "fan") {
        sendControlCommand({
            FanMode: "manual",
            FanSwitch: controlState[device]
        });
    } else {
        sendControlCommand({
            MotorMode: "manual",
            MotorSwitch: controlState[device]
        });
    }
}

function initControls() {
    ["fan", "atomizer", "motor"].forEach(device => {
        updateControlButton(device);
        document.getElementById(`${device}Control`).addEventListener("click", () => {
            handleControlToggle(device);
        });
    });
    updateGlobalModeButton();
    document.getElementById("globalModeToggle").addEventListener("click", () => {
        setGlobalMode(globalMode === "auto" ? "manual" : "auto");
    });
}

document.addEventListener("DOMContentLoaded", () => {
    initControls();
    updateUI(initialData);
    fetchOneNETData();
    setInterval(fetchOneNETData, 5000);
});

