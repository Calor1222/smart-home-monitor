# 腾讯云函数 OneNET 代理

这个目录提供一个腾讯云 SCF 云函数，用来代理请求 OneNET 最新设备属性，避免把 OneNET 密钥直接暴露到前端。

## 返回格式

云函数成功时返回：

```json
{
  "Temp": 24,
  "Hum": 37,
  "Smoke": 48,
  "pm": 6.2,
  "time": "2026-05-05 12:00:00"
}
```

如果 OneNET 返回结构和预期不一致，会返回：

```json
{
  "error": "OneNET response parse failed",
  "raw": {}
}
```

这样方便前期调试接口。

## 环境变量

需要在腾讯云函数控制台里配置这些环境变量：

- `PRODUCT_ID`
- `DEVICE_NAME`
- `ONENET_AUTH`

示例见 [`.env.example`](./.env.example)。

## OneNET 接口

云函数内部请求：

```txt
https://iot-api.heclouds.com/thingmodel/query-device-property?product_id=PRODUCT_ID&device_name=DEVICE_NAME
```

请求头：

```txt
Authorization: ONENET_AUTH
Accept: application/json
```

## 部署步骤

1. 在腾讯云控制台进入“云函数 SCF”。
2. 新建函数，运行环境选择 `Node.js`。
3. 创建 `Web Function` 或开启函数 URL / API 网关触发。
4. 把本目录下的 `index.js` 和 `package.json` 上传到函数代码里。
5. 在“环境变量”中配置：
   - `PRODUCT_ID`
   - `DEVICE_NAME`
   - `ONENET_AUTH`
6. 保存并部署。
7. 复制云函数的访问 URL。
8. 浏览器访问该 URL，确认能返回 JSON。

## CORS 说明

代码已默认开启：

- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type, Authorization`

适合 GitHub Pages 这类静态站点直接访问。

## 前端接入方式

后续在前端 `web/app.js` 中，把 `fetchOneNETData()` 改成下面这种形式：

```js
async function fetchOneNETData() {
    const response = await fetch("https://你的云函数访问地址");
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
    }
    return response.json();
}
```

然后把返回值整理成当前页面需要的数据结构即可。

## 注意事项

- 不要把 `ONENET_AUTH` 写进前端代码。
- 如果 OneNET 属性名有变动，可以在 `index.js` 里的 `normalizePropertyData()` 里补充映射。
- 当前使用 Node.js 原生 `https`，没有引入 `axios` 等第三方依赖，部署更轻量。
