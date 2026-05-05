const TEXT = {
    online: "\u5728\u7ebf",
    offline: "\u79bb\u7ebf",
    noData: "--",
    pmLevelPrefix: "\u7a7a\u6c14\u8d28\u91cf\u7b49\u7ea7\uff1a",
    good: "\u4f18",
    fair: "\u826f",
    light: "\u8f7b\u5ea6\u6c61\u67d3",
    danger: "\u91cd\u5ea6\u6c61\u67d3"
};

const mockData = {
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

const controlState = {
    fan: false,
    atomizer: false,
    motor: false
};

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
        labels: limitedHistory.map(item => item.time || TEXT.noData),
        datasets: [
            {
                label: "Temp",
                data: limitedHistory.map(item => item.Temp),
                borderColor: "#555555",
                borderWidth: 1,
                pointRadius: 2,
                tension: 0
            },
            {
                label: "Hum",
                data: limitedHistory.map(item => item.Hum),
                borderColor: "#777777",
                borderWidth: 1,
                pointRadius: 2,
                tension: 0
            },
            {
                label: "Smoke",
                data: limitedHistory.map(item => item.Smoke),
                borderColor: "#999999",
                borderWidth: 1,
                pointRadius: 2,
                tension: 0
            },
            {
                label: "pm",
                data: limitedHistory.map(item => item.pm),
                borderColor: "#222222",
                borderWidth: 1,
                pointRadius: 2,
                tension: 0
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

function renderDashboard(data) {
    updateStatusBar(data);
    updateCards(data);
    updateChart(data.history);
}

async function fetchOneNETData() {
    // 这里预留后续接入 OneNET HTTP API 的逻辑
    // 前端不要写死 Token，建议后续通过后端接口转发请求
    return mockData;
}

function sendControlCommand(device, state) {
    console.log("sendControlCommand", device, state);
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

function handleControlToggle(device) {
    controlState[device] = !controlState[device];
    updateControlButton(device);
    sendControlCommand(device, controlState[device]);
}

function initControls() {
    ["fan", "atomizer", "motor"].forEach(device => {
        updateControlButton(device);
        document.getElementById(`${device}Control`).addEventListener("click", () => {
            handleControlToggle(device);
        });
    });
}

async function refreshData() {
    try {
        const data = await fetchOneNETData();
        renderDashboard(data);
    } catch (error) {
        console.error("Load dashboard data failed:", error);
    }
}

document.addEventListener("DOMContentLoaded", () => {
    initControls();
    renderDashboard(mockData);
    setInterval(refreshData, 5000);
});
