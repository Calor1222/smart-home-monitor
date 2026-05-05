const https = require('https');

function createResponse(statusCode, data) {
    return {
        statusCode,
        headers: {
            'Content-Type': 'application/json; charset=utf-8',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
            'Access-Control-Allow-Headers': '*'
        },
        body: JSON.stringify(data)
    };
}

function getPropValue(props, key) {
    if (!Array.isArray(props)) {
        return 0;
    }

    const item = props.find(p => p.identifier === key || p.name === key);
    return item ? Number(item.value) : 0;
}

exports.main_handler = async (event, context) => {
    if (event && event.path === '/env-check') {
        return createResponse(200, {
            PRODUCT_ID: process.env.PRODUCT_ID || null,
            DEVICE_NAME: process.env.DEVICE_NAME || null,
            ONENET_AUTH_LENGTH: process.env.ONENET_AUTH ? process.env.ONENET_AUTH.length : 0
        });
    }

    if (event && event.path === '/ping') {
        return createResponse(200, { ok: true });
    }

    if (event && event.httpMethod === 'OPTIONS') {
        return createResponse(200, { ok: true });
    }

    const PRODUCT_ID = process.env.PRODUCT_ID;
    const DEVICE_NAME = process.env.DEVICE_NAME;
    const ONENET_AUTH = process.env.ONENET_AUTH;

    if (!PRODUCT_ID || !DEVICE_NAME || !ONENET_AUTH) {
        return createResponse(500, {
            error: 'missing env',
            message: '请检查 PRODUCT_ID / DEVICE_NAME / ONENET_AUTH 环境变量'
        });
    }

    const apiUrl =
        `https://iot-api.heclouds.com/thingmodel/query-device-property?product_id=${encodeURIComponent(PRODUCT_ID)}&device_name=${encodeURIComponent(DEVICE_NAME)}`;

    return new Promise((resolve) => {
        const request = https.get(apiUrl, {
            headers: {
                'Authorization': ONENET_AUTH,
                'Accept': 'application/json'
            }
        }, (onenetRes) => {
            let raw = '';

            onenetRes.on('data', chunk => {
                raw += chunk;
            });

            onenetRes.on('end', () => {
                try {
                    const json = JSON.parse(raw);

                    if (json.code !== 0 && json.errno !== 0) {
                        resolve(createResponse(500, {
                            error: 'onenet error',
                            raw: json
                        }));
                        return;
                    }

                    const props = json.data || [];

                    resolve(createResponse(200, {
                        Temp: getPropValue(props, 'Temp'),
                        Hum: getPropValue(props, 'Hum'),
                        Smoke: getPropValue(props, 'Smoke'),
                        pm: getPropValue(props, 'pm'),
                        time: new Date().toLocaleString()
                    }));
                } catch (err) {
                    resolve(createResponse(500, {
                        error: 'parse error',
                        message: err.message,
                        raw
                    }));
                }
            });
        });

        request.on('error', (err) => {
            resolve(createResponse(500, {
                error: 'request error',
                message: err.message
            }));
        });

        request.setTimeout(8000, () => {
            request.destroy();
            resolve(createResponse(504, {
                error: 'timeout',
                message: '请求 OneNET 超时'
            }));
        });
    });
};
