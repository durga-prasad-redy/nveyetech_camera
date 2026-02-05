# JULY 2025 - Motocam APIs

## motocam_apis 2.0.0



# 📡 Motocam HTTP API Documentation — Updated (July 2025)

This document describes the HTTP endpoints, session handling, and proxy forwarding logic used in the Motocam API server.

## 🔐 Authentication

Session management uses a cookie named `session`. A valid session cookie is required for most API endpoints. Sessions are created using a 4-digit PIN via `/api/login`.

- **Max Sessions**: `3`
- **Timeout**: `3600` seconds (1 hour)
- **Cookie**: HTTP-only
- **Secure flag**: Disabled (set only when using HTTPS)


---

## 📌 Endpoints

### `POST /api/login`

Authenticate the client using a 4-digit PIN.

#### Request

```json
{
  "pin": "1234"
}
```

**Content-Type**: `application/json`

#### Response

* `200 OK`: Returns a session cookie.
* `400 Bad Request`: PIN missing or wrong format.
* `403 Forbidden`: Invalid PIN.
* `500 Internal Server Error`: Session creation failed.

---

### `GET /api/logout`

Logs out the current session.

#### Response

* `200 OK`: Clears the session cookie and confirms logout.

---

### `POST /api/motocam_api`

Main Motocam API endpoint for sending raw hex data.

#### Request

**Content-Type**: `application/octet-stream`

Send data as a space-separated hex string:

```
0xA1 0xB2 0xC3 ...
```



###  `POST /api/upload`

Used to upload OTA firmware files to the device.

* `Destination` : /mnt/flash/vienna/firmware/ota

* `Size Limit` : 4 MB

### Response

* `200 OK`: File uploaded.

* `401 Unauthorized`: Missing/invalid session.



#### Response

Returns the response as a space-separated hex string.

**Status Codes**:

* `200 OK`: Success
* `401 Unauthorized`: Missing or invalid session
* `415 Unsupported Media Type`: Content-Type must be `application/octet-stream`

---

### `GET /getdeviceid`

**Redirects to**: `http://0.0.0.0:8083/getdeviceid`

---

### `GET /getsignalserver`

**Redirects to**: `http://0.0.0.0:8083/getsignalserver`

---

## 🌐 Web UI

Static files are served from:

* `dist/` (Linux/Windows)
* `/web_root` (embedded/firmware)

Fallback to `index.html` if file not found.

---






## 📡 mDNS Support

The following `.local` hostnames are broadcast:

* `{device_name}.local` (Ethernet interface)
* `{device_name}_wifi.local`(WiFi interface)



---

## ⚙️ Settings and Configuration

Stored in a `settings` structure:

```c
struct settings {
    bool log_enabled;
    int log_level;
    long brightness;
    char device_name[50];
    char wifi_hotspot_ipaddress[50];
    char wifi_client_ipaddress[50];
};
```

Populated on `web_init(...)` call.

---

## 🔄 Session Management

Session creation, validation, and invalidation handled using `SessionManager`.

* Max sessions: `3`
* Timeout: `3600s`
* HTTP-only cookie used
* Secure flag is **disabled** (enable for HTTPS only deployments)

---


### Request Packet Format:

| Header | Command | Sub-command | Data Length | Data          | CRC    |
| ------ | ------- | ----------- | ----------- | ------------- | ------ |
| 1 byte | 1 byte  | 1 byte      | 1 byte      | Max 255 bytes | 1 byte |

### Response Packet Format:

| Header | Command | Sub-command | Data Length (Success/Failed flag + Data/Error Code) | Success/Failed flag | Data/Error Code | CRC    |
| ------ | ------- | ----------- | --------------------------------------------------- | ------------------- | --------------- | ------ |
| 1 byte | 1 byte  | 1 byte      | 1 byte                                              | 1 byte              | Max 254 bytes   | 1 byte |

### Packet Description:

#### Header:

| Set | Get | Ack | Response |
| --- | --- | --- | -------- |
| 1   | 2   | 3   | 4        |

#### Commands:

| Streaming | Network | Config | Image | Reserved for internal use | System |
| --------- | ------- | ------ | ----- | ------------------------- | ------ |
| 1         | 2       | 3      | 4     | 5                         | 6      |

#### Streaming Sub Commands:

| Start |     |
| ----- | --- |
| 1     |     |

#### Network Sub Commands:

| Wifi Hotspot | Wifi Client | Wifi State | Ethernet | Onvif |
| ------------ | ----------- | ---------- | -------- | ----- |
| 1            | 2           | 3          | 4        | 5     |

#### Config Set Sub Commands:

| Default to Factory | Default to Current | Current to Factory | Current to Default |
| ------------------ | ------------------ | ------------------ | ------------------ |
| 9                  | 11                 | 13                 | 14                 |

#### Config Get Sub Commands:

| Factory |     |     | Default |     |     | Current |     |     | Streaming Config |
| ------- | --- | --- | ------- | --- | --- | ------- | --- | --- | ---------------- |
| 4       |     |     | 8       |     |     | 12      |     |     | 10               |

#### Image Sub Commands:

| ZOOM | ROTATION | IR Cut Filter | IR Brightness | Day Mode | Resolution | Mirror | Flip | TILT | WDR | EIS |
| ---- | -------- | ------------- | ------------- | -------- | ---------- | ------ | ---- | ---- | --- | --- |
| 1    | 2        | 3             | 4             | 5        | 6          | 7      | 8    | 9    | 10  | 11  |

| GyroReader | ROTAMISCION |
| ---------- | ----------- |
| 12         | 13          |


#### System Sub Commands:

| Get: Camera Name | Get: Firmware | Get: MAC Address | Get: Login PIN | Get: OTA status | Health check |
| ---------------- | ------------- | ---------------- | -------------- | --------------- | ------------ |
| 1                | 2             | 3                | 4              | 5               | 6            |

| Set: Camera Name | Set: Login PIN | Set: Factory Reset | Set: Shutdown | Set: OTA update |
| ---------------- | -------------- | ------------------ | ------------- | --------------- |
| 1                | 2              | 3                  | 4             | 5               |

'0' will be sent when sub commands are not required.

#### Data:

| IR Brightness |      |
| ------------- | ---- |
| 0             | 100% |
| 0             | 1    |

| Image Resolution     |                      |
| -------------------- | -------------------- |
| 0-3840 x 2160 pixels | 1-1920 x 1080 pixels |
| 0                    | 1                    |

| IR Cut Filter |     |
| ------------- | --- |
| OFF           | ON  |
| 0             | 1   |

| Mirror |     |
| ------ | --- |
| OFF    | ON  |
| 0      | 1   |

| Flip |     |
| ---- | --- |
| OFF  | ON  |
| 0    | 1   |

| WDR- Wide Dynamic Range |     |
| ----------------------- | --- |
| OFF                     | ON  |
| 0                       | 1   |

| EIS - Electronic Image Stabilization |     |
| ------------------------------------ | --- |
| OFF                                  | ON  |
| 0                                    | 1   |

| Wifi Encryption Type |     |
| -------------------- | --- |
| WEP                  | WPA |
| 1                    | 2   |

| Command Acknowledgement |        |
| ----------------------- | ------ |
| 0                       | 1      |
| Success                 | Failed |

| Error Code                     |                       |                 |                     |                          |                    |
| ------------------------------ | --------------------- | --------------- | ------------------- | ------------------------ | ------------------ |
| -1                             | -2                    | -3              | -4                  | -5                       | -6                 |
| Error in executing the command | Invalid packet header | Invalid command | Invalid sub-command | Invalid Data/Data Length | CRC does not match |

# Packet Communication

- Use Camera IP address
<!-- - Use Port 9090 -->

NB For the first access, default SSID, default encryption type & key, default IP and default Subnet mask are to be used.

| Packet Field Description          |     |
| --------------------------------- | --- |
| Packet Value Description          |     |
| Packet Value (Actual Packet Data) |     |

## Streaming commands:


### GET STREAMING STATUS:

| Header | Command   | Sub-command | Data Length | Data | CRC    |
| ------ | --------- | ----------- | ----------- | ---- | ------ |
| Get    | Streaming | status      | 0 bytes     | -    | 1 byte |
| 2      | 1         | 1           | 0           | -    | 1 byte |



#### get Streaming status Success Response

##### Main Response Structure

| Header   | Command   | Sub-command      | Data Length | Success/Failed Flag | status |
| -------- | --------- | ---------------- | ----------- | ------------------- | ------ |
| Response | Streaming | streaming_status | 2 byte      | Success (0)         | 1/0    |



#### Streaming_config Error Response:

| Header   | Command   | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | --------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Response | Streaming | streaming_status | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 1         | 1                | 2           | 1                   | -1 to -6   | 1 byte |



### GET WEBRTC STREAMING STATUS:

| Header | Command   | Sub-command   | Data Length | Data | CRC    |
| ------ | --------- | ------------- | ----------- | ---- | ------ |
| Get    | Streaming | webrtc_status | 0 bytes     | -    | 1 byte |
| 2      | 1         | 2             | 0           | -    | 1 byte |

#### get portablertc Streaming status Success Response

##### Main Response Structure

| Header   | Command   | Sub-command   | Data Length | Success/Failed Flag | status |
| -------- | --------- | ------------- | ----------- | ------------------- | ------ |
| Response | Streaming | webrtc_status | 2 byte      | Success (0)         | 1/0    |



#### Streaming_config Error Response:

| Header   | Command   | Sub-command   | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | --------- | ------------- | ----------- | ------------------- | ---------- | ------ |
| Response | Streaming | webrtc_status | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 1         | 2             | 2           | 1                   | -1 to -6   | 1 byte |







### START WEBRTC  STREAMING STATUS:

| Header | Command   | Sub-command  | Data Length | Data | CRC    |
| ------ | --------- | ------------ | ----------- | ---- | ------ |
| SET    | Streaming | webrtc_start | 0 bytes     | -    | 1 byte |
| 2      | 1         | 3            | 0           | -    | 1 byte |

#### START WEBRTC Streaming status Success Response

##### Main Response Structure

| Header   | Command   | Sub-command  | Data Length | Success/Failed Flag | status |
| -------- | --------- | ------------ | ----------- | ------------------- | ------ |
| Response | Streaming | webrtc_start | 2 byte      | Success (0)         | 1/0    |



#### START WEBRTC Error Response:

| Header | Command   | Sub-command  | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | --------- | ------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | STREAMING | START WEBRTC | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 1         | 3            | 2           | 1                   | -1 to -6   | 1 byte |




### STOP WEBRTC  STREAMING STATUS:

| Header | Command   | Sub-command | Data Length | Data | CRC    |
| ------ | --------- | ----------- | ----------- | ---- | ------ |
| SET    | Streaming | webrtc_stop | 0 bytes     | -    | 1 byte |
| 2      | 1         | 4           | 0           | -    | 1 byte |

#### STOP WEBRTC Streaming status Success Response

##### Main Response Structure

| Header   | Command   | Sub-command | Data Length | Success/Failed Flag | status |
| -------- | --------- | ----------- | ----------- | ------------------- | ------ |
| Response | Streaming | webrtc_stop | 2 byte      | Success (0)         | 1/0    |



#### STOP WEBRTC Error Response:

| Header | Command   | Sub-command  | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | --------- | ------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | STREAMING | START WEBRTC | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 1         | 4            | 2           | 1                   | -1 to -6   | 1 byte |








### Restart Streaming:

| Header | Command   | Sub-command | Data Length | Data | CRC    |
| ------ | --------- | ----------- | ----------- | ---- | ------ |
| Set    | Streaming | Start       | 0 bytes     | -    | 1 byte |
| 1      | 1         | 1           | 0           | -    | 1 byte |

#### Restart Streaming Success Ack:

| Header | Command   | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | --------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Streaming | Start       | 1 byte      | Success             | Success | 1 byte |
| 3      | 1         | 1           | 2           | 0                   | 0       | 1 byte |

#### Restart Streaming Error Ack:

| Header | Command   | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | --------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Streaming | Start       | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 1         | 1           | 2           | 1                   | -1 to -6   | 1 byte |

## Config Commands:

<!-- ### Set Default to Factory:

| Header | Command | Sub-command        | Data Length | Data | CRC    |
| ------ | ------- | ------------------ | ----------- | ---- | ------ |
| Set    | Config  | Default to Factory | 0 bytes     | -    | 1 byte |

#### Set Default to Factory Success Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ------- | ------ |
| Ack    | Config  | Default to Factory | 1 byte      | Success             | Success | 1 byte |
| 3      | 3       | 9                  | 2           | 0                   | 0       | 1 byte |

#### Set Default to Factory Error Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | Config  | Default to Factory | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 3       | 9                  | 2           | 1                   | -1 to -6   | 1 byte |

### Set Default to Current:

| Header | Command | Sub-command        | Data Length | Data | CRC    |
| ------ | ------- | ------------------ | ----------- | ---- | ------ |
| Set    | Config  | Default to Current | 0 bytes     | -    | 1 byte |
| 1      | 3       | 11                 | 0           | -    | 1 byte |

#### Set Default to Current Success Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ------- | ------ |
| Ack    | Config  | Default to Current | 1 byte      | Success             | Success | 1 byte |
| 3      | 3       | 11                 | 2           | 0                   | 0       | 1 byte |

#### Set Default to Current Error Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | Config  | Default to Current | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 3       | 11                 | 2           | 1                   | -1 to -6   | 1 byte |

### Set Current to Factory:

| Header | Command | Sub-command        | Data Length | Data | CRC    |
| ------ | ------- | ------------------ | ----------- | ---- | ------ |
| Set    | Config  | Current to Factory | 0 bytes     | -    | 1 byte |

| 1 | 3 | 13 | 0 | - | 1 byte |

#### Set Current to Factory Success Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ------- | ------ |
| Ack    | Config  | Current to Factory | 1 byte      | Success             | Success | 1 byte |
| 3      | 3       | 13                 | 2           | 0                   | 0       | 1 byte |

#### Set Current to Factory Error Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | Config  | Current to Factory | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 3       | 13                 | 2           | 0                   | -1 to -6   | 1 byte |

### Set Current to Default:

| Header | Command | Sub-command        | Data Length | Data | CRC    |
| ------ | ------- | ------------------ | ----------- | ---- | ------ |
| Set    | Config  | Current to Default | 0 bytes     | -    | 1 byte |
| 1      | 3       | 14                 | 0           | -    | 1 byte |

#### Set Current to Default Success Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ------- | ------ |
| Ack    | Config  | Current to Default | 1 byte      | Success             | Success | 1 byte |
| 3      | 3       | 14                 | 2           | 0                   | 0       | 1 byte |

#### Set Current to Default Error Ack:

| Header | Command | Sub-command        | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | Config  | Current to Default | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 3       | 14                 | 2           | 1                   | -1 to -6   | 1 byte | --> |

### Get Config Request:

| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Get    | Config  | Current     | 0 bytes     | -    | 1 byte |

| 2 | 3 | 12 | 0 | - | 1 byte |

#### Get Config Success Response:

| Header   | Command | Sub-command             | Data Length | Success/Failed flag | Data              |                    |               |                   |
| -------- | ------- | ----------------------- | ----------- | ------------------- | ----------------- | ------------------ | ------------- | ----------------- |
| Response | Config  | Factory/Default/Current | 1 byte      | Success             | Image Zoom        | Image Rotation     | IR Cut Filter | IR Brightness     |
|          |         |                         |             |                     | x1 / x2 / x3 / x4 | 0 / 90 / 180 / 270 | On/Off        | 0 / 50 / 75 / 100 |
| 4        | 3       | 4 / 8 / 12              | 12          | 0                   | 1 / 2 / 3 / 4     | 1 / 2 / 3 / 4      | 1 / 0         | 0 / 1 / 2 / 3     |

| DATA     |                     |        |        |                  |        |        |             |      |        | CRC    |
| -------- | ------------------- | ------ | ------ | ---------------- | ------ | ------ | ----------- | ---- | ------ | ------ |
| DAY MODE | Resolution          | Mirror | Flip   | Tilt             | WDR    | EIS    | gyro_reader | MISC | MIC    | 1 byte |
| On/Off   | 3840x2160/1920x1080 | On/Off | On/Off | 0/15/30/45/60/90 | On/Off | On/Off |             |      | On/Off |        |
| 1/0      | 0 / 1               | 1 / 0  | 1 / 0  | 0/1/2/3/4/5      | 1 / 0  | 1 / 0  | 1/0         | 0-12 | 1 / 0  | 1 byte |

#### Get Config Error Response:

| Header   | Command | Sub-command             | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------------------- | ----------- | ------------------- | ---------- | ------ |
| Response | Config  | Factory/Default/Current | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 3       | 4 / 8 / 12              | 2           | 1                   | -1 to -6   | 1 byte |



### Get Streaming_config Request:

| Header | Command | Sub-command      | Data Length | Data | CRC    |
| ------ | ------- | ---------------- | ----------- | ---- | ------ |
| Get    | Config  | streaming_config | 0 bytes     | -    | 1 byte |
| 2      | 3       | 10               | 0           | -    | 1 byte |

#### Streaming Configuration Success Response

##### Main Response Structure

| Header   | Command | Sub-command      | Data Length | Success/Failed Flag |
| -------- | ------- | ---------------- | ----------- | ------------------- |
| Response | Config  | streaming_config | 1 byte      | Success (0)         |

##### Data Parameters

| Parameter          | Values                         | Description               |
| ------------------ | ------------------------------ | ------------------------- |
| stream1_resolution | 1 = R960x540<br>2 = R1920x1080 | Video resolution options  |
| stream1_fps        | 1-30                           | Frames per second (range) |
| stream1_bitrate    | 1-8                            | Bitrate level             |
| stream1_encoder    | 1 = H264<br>2 = H265           | Video encoder options     |



| Data                     |             |                 |                 |        |       |       |        | CRC |
| ------------------------ | ----------- | --------------- | --------------- | ------ | ----- | ----- | ------ | --- |
| stream2_resolution       | stream2_fps | stream2_bitrate | stream2_encoder | 1 byte |       |
| R960x540 =1 R1920x1080=2 | 1-30        | 1-8             | H264 =1 H265=2  |
| 1 / 2                    | 1-30        | 1-8             | 0/1/2           | 1 / 0  | 1 / 0 | 1 / 0 | 1 byte |     |

#### Streaming_config Error Response:

| Header   | Command | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Response | Config  | streaming_config | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 3       | 10               | 2           | 1                   | -1 to -6   | 1 byte |




# Image Commands:

### Set IR Brightness Value:

| Header | Command | Sub-command         | Data Length | Data     | CRC    |
| ------ | ------- | ------------------- | ----------- | -------- | ------ |
| Set    | Image   | Image IR Brightness | 1 byte      | 0 / 100% | 1 byte |

| 1 | 4 | 4 | 1 | 0 / 1 | 1 byte |

#### Set IR Brightness Success Ack:

| Header | Command | Sub-command   | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | IR Brightness | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 4             | 2           | 0                   | 0       | 1 byte |

#### Set IR Brightness Error Ack:

| Header | Command | Sub-command   | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | IR Brightness | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 4             | 2           | 1                   | -1 to -6   | 1 byte |


### Set MID IR Brightness Value:

| Header | Command | Sub-command         | Data Length | Data     | CRC    |
| ------ | ------- | ------------------- | ----------- | -------- | ------ |
| Set    | Image   | Image IR Brightness | 1 byte      | 0 / 100% | 1 byte |

| 1 | 4 | 4 | 1 | 0 / 1 | 1 byte |

#### Set MID IR Brightness Success Ack:

| Header | Command | Sub-command       | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | MID IR Brightness | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 14                | 2           | 0                   | 0       | 1 byte |

#### Set MID IR Brightness Error Ack:

| Header | Command | Sub-command       | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | MID IR Brightness | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 14                | 2           | 1                   | -1 to -6   | 1 byte |



### Set SIDE IR Brightness Value:

| Header | Command | Sub-command         | Data Length | Data     | CRC    |
| ------ | ------- | ------------------- | ----------- | -------- | ------ |
| Set    | Image   | Image IR Brightness | 1 byte      | 0 / 100% | 1 byte |

| 1 | 4 | 4 | 1 | 0 / 1 | 1 byte |

#### Set SIDE IR Brightness Success Ack:

| Header | Command | Sub-command       | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | MID IR Brightness | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 15                | 2           | 0                   | 0       | 1 byte |

#### Set SIDE IR Brightness Error Ack:

| Header | Command | Sub-command       | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | MID IR Brightness | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 15                | 2           | 1                   | -1 to -6   | 1 byte |


### Set Resolution:

| Header | Command | Sub-command      | Data Length | Data                      | CRC    |
| ------ | ------- | ---------------- | ----------- | ------------------------- | ------ |
| Set    | Image   | Image Resolution | 1 byte      | 3840 x 2160 / 1920 x 1080 | 1 byte |
| 1      | 4       | 6                | 1           | 0 / 1                     | 1 byte |

#### Set Resolution Success Ack:

| Header | Command | Sub-command      | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ---------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | Image Resolution | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 6                | 2           | 0                   | 0       | 1 byte |

#### Set Resolution Error Ack:

| Header | Command | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | Image Resolution | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 6                | 2           | 1                   | -1 to -6   | 1 byte |

### Set IR Cut Filter On/Off:

| Header | Command | Sub-command   | Data Length | Data   | CRC    |
| ------ | ------- | ------------- | ----------- | ------ | ------ |
| Set    | Image   | IR Cut Filter | 1 byte      | Off/On | 1 byte |
| 1      | 4       | 3             | 1           | 0 / 1  | 1 byte |

#### Set IR Cut Filter On/Off Success Ack:

| Header | Command | Sub-command   | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | IR Cut Filter | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 3             | 2           | 0                   | 0       | 1 byte |

#### Set IR Cut Filter On/Off Error Ack:

| Header | Command | Sub-command   | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | IR Cut Filter | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 3             | 2           | 1                   | -1 to -6   | 1 byte |

### Set Mirror On/Off:

| Header | Command | Sub-command | Data Length | Data   | CRC    |
| ------ | ------- | ----------- | ----------- | ------ | ------ |
| Set    | Image   | Mirror      | 1 byte      | 0 / 1  | 1 byte |
| 1      | 4       | 7           | 1           | Off/On | 1 byte |

#### Set Mirror Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | Mirror      | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 7           | 2           | 0                   | 0       | 1 byte |

#### Set Mirror Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | Mirror      | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 7           | 2           | 1                   | -1 to -6   | 1 byte |

### Set Flip On/Off:

| Header | Command | Sub-command | Data Length | Data   | CRC    |
| ------ | ------- | ----------- | ----------- | ------ | ------ |
| Set    | Image   | Flip        | 1 byte      | Off/On | 1 byte |
| 1      | 4       | 8           | 1           | 0 / 1  | 1 byte |

#### Set Flip On/Off Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | Flip        | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 8           | 2           | 0                   | 0       | 1 byte |

#### Set Flip On/Off Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | Flip        | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 8           | 2           | 1                   | -1 to -6   | 1 byte |

### Set WDR On/Off:

| Header | Command | Sub-command | Data Length | Data   | CRC    |
| ------ | ------- | ----------- | ----------- | ------ | ------ |
| Set    | Image   | WDR         | 1 byte      | Off/On | 1 byte |
| 1      | 4       | 10          | 1           | 0 / 1  | 1 byte |

#### Set WDR On/Off Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | WDR         | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 10          | 2           | 0                   | 0       | 1 byte |

#### Set WDR On/Off Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | WDR         | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 10          | 2           | 1                   | -1 to -6   | 1 byte |

### Set EIS On/Off:

| Header | Command | Sub-command | Data Length | Data   | CRC    |
| ------ | ------- | ----------- | ----------- | ------ | ------ |
| Set    | Image   | EIS         | 1 byte      | Off/On | 1 byte |

| 1 | 4 | 11 | 1 | 0 / 1 | 1 byte |

#### Set EIS On/Off Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Image   | EIS         | 1 byte      | Success             | Success | 1 byte |
| 3      | 4       | 11          | 2           | 0                   | 0       | 1 byte |

#### Set EIS On/Off Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Image   | EIS         | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 4       | 11          | 2           | 1                   | -1 to -6   | 1 byte |

Note: To get IR Brightness, Resolution, Ircutfilter, Mirror, Flip, WDR, EIS values use "Get Config Request" (with sub-command "Current")

Note: Changing Resolution, EIS, and WDR command will change the encoder pipeline, we need to restart the streaming app to apply these changes.

## Network commands:

### Set WifiHotspot Configuration

#### Main Command Structure

| Header | Command | Sub-command  | Data Length |
| ------ | ------- | ------------ | ----------- |
| Set    | Network | Wifi Hotspot | 1 byte      |

#### Data Parameters

| Parameter             | Values / Format                     | Description               |
| --------------------- | ----------------------------------- | ------------------------- |
| SSID Length           | 1-32 bytes                          | Length of the SSID string |
| SSID                  | ASCII string (1-32B)                | Network name              |
| Encryption Type       | 1 = Open<br>2 = WPA2<br>3 = WPA3    | Security mode             |
| Encryption Key Length | 1-32 bytes                          | Length of the password    |
| Encryption Key        | ASCII string (1-32B)                | Network password          |
| IP Address            | IPv4 format (e.g., `192.168.1.123`) | Hotspot IP                |
| Subnet Mask           | IPv4 format (e.g., `255.255.255.0`) | Network mask              |

##### Example Command

| Header | Command | Sub-command | Data Length | SSID Len | SSID   | Enc Type | Key Len | Enc Key   | IP Address    | Subnet Mask   | CRC |
| ------ | ------- | ----------- | ----------- | -------- | ------ | -------- | ------- | --------- | ------------- | ------------- | --- |
| 1      | 2       | 1           | 5-133       | 1-32     | MySSID | 2        | 8       | mypass123 | 192.168.1.123 | 255.255.255.0 | 1   |

###### Notes:
- **Data Length**: Varies (5–133 bytes) based on SSID/Key lengths.
- **Encryption Types**:
  - `1` = Open (no auth)
  - `2` = WPA2
  - `3` = WPA3
- **CRC**: Checksum (1 byte).

#### Set WifiHotspot Success Ack:

| Header | Command | Sub-command  | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------ | ----------- | ------------------- | ------- | ------ |
| Ack    | Network | Wifi Hotspot | 1 byte      | Success             | Success | 1 byte |
| 3      | 2       | 1            | 2           | 0                   | 0       | 1 byte |

#### Set WifiHotspot Error Ack:

| Header | Command | Sub-command  | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------ | ----------- | ------------------- | ---------- | ------ |
| Ack    | Network | Wifi Hotspot | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 2       | 1            | 2           | 1                   | -1 to -6   | 1 byte |

### Set WifiClient Configuration

#### Main Command Structure

| Header | Command | Sub-command | Data Length |
| ------ | ------- | ----------- | ----------- |
| Set    | Network | Wifi Client | 1 byte      |

#### Data Parameters

| Parameter              | Values / Format                     | Description                    |
| ---------------------- | ----------------------------------- | ------------------------------ |
| SSID Length            | 1-32 bytes                          | Length of the SSID string      |
| SSID                   | ASCII string (1-32B)                | Network name to connect to     |
| Encryption Type        | 1 = Open<br>2 = WPA2<br>3 = WPA3    | Security mode                  |
| Encryption Key Length  | 1-32 bytes                          | Length of the password         |
| Encryption Key         | ASCII string (1-32B)                | Network password               |
| IP Address (Optional)  | IPv4 format (e.g., `192.168.1.123`) | *Static IP (if DHCP disabled)* |
| Subnet Mask (Optional) | IPv4 format (e.g., `255.255.255.0`) | *Required for static IP*       |

##### Example Command

| Header | Command | Sub-command | Data Length | SSID Len | SSID      | Enc Type | Key Len | Enc Key   | IP Address    | Subnet Mask   | CRC |
| ------ | ------- | ----------- | ----------- | -------- | --------- | -------- | ------- | --------- | ------------- | ------------- | --- |
| 1      | 2       | 2           | 5-133       | 8        | MyWiFiNet | 2        | 10      | Secure123 | 192.168.1.123 | 255.255.255.0 | 1   |

###### Notes:
- **Data Length**: Varies (5–133 bytes) based on SSID/Key lengths.
- **Encryption Types**:
  - `1` = Open (no auth)
  - `2` = WPA2
  - `3` = WPA3
- **IP/Subnet Mask**: Optional (only for static IP; omit for DHCP).
- **CRC**: Checksum (1 byte).

#### Set WifiClient Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Network | Wifi Client | 1 byte      | Success             | Success | 1 byte |
| 3      | 2       | 2           | 2           | 0                   | 0       | 1 byte |

#### Set WifiClient Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Network | Wifi Client | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 2       | 2           | 2           | 1                   | -1 to -5   | 1 byte |

### Get Wifi Hotspot Request:

| Header | Command | Sub-command  | Data Length | Data | CRC    |
| ------ | ------- | ------------ | ----------- | ---- | ------ |
| Get    | Network | Wifi Hotspot | 0 bytes     | -    | 1 byte |
| 2      | 2       | 1            | 0           | -    | 1 byte |

### Get Wifi Hotspot Success Response

#### Response Structure

| Header   | Command | Sub-command  | Data Length | Success Flag |
| -------- | ------- | ------------ | ----------- | ------------ |
| Response | Network | Wifi Hotspot | 1 byte      | 0 (Success)  |

#### Response Data Parameters

| Parameter             | Values / Format                     | Description               |
| --------------------- | ----------------------------------- | ------------------------- |
| SSID Length           | 1-32 bytes                          | Length of the SSID string |
| SSID                  | ASCII string (1-32B)                | Network name              |
| Encryption Type       | 1 = Open<br>2 = WPA2<br>3 = WPA3    | Security mode             |
| Encryption Key Length | 1-32 bytes                          | Length of the password    |
| Encryption Key        | ASCII string (1-32B)                | Network password          |
| IP Address            | IPv4 format (e.g., `192.168.1.123`) | Hotspot IP                |
| Subnet Mask           | IPv4 format (e.g., `255.255.255.0`) | Network mask              |

##### Example Success Response

| Header | Command | Sub-command | Data Length | Success | SSID Len | SSID   | Enc Type | Key Len | Enc Key   | IP Address    | Subnet Mask   | CRC |
| ------ | ------- | ----------- | ----------- | ------- | -------- | ------ | -------- | ------- | --------- | ------------- | ------------- | --- |
| 4      | 2       | 1           | 5-134       | 0       | 8        | MySSID | 2        | 8       | mypass123 | 192.168.1.123 | 255.255.255.0 | 1   |

##### Notes:
- **Success Flag**: `0` indicates success
- **Data Length**: Varies (5-134 bytes) based on SSID/Key lengths
- **Encryption Types**:
  - `1` = Open (no auth)
  - `2` = WPA2
  - `3` = WPA3
- **CRC**: Checksum (1 byte)

#### Get Wifi Hotspot Error Response:

| Header   | Command | Sub-command  | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ------------ | ----------- | ------------------- | ---------- | ------ |
| Response | Network | Wifi Hotspot | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 2       | 2            | 2           | 1                   | -1 to -6   | 1 byte |

### Get Wifi Client Request:

| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Get    | Network | Wifi Client | 0 bytes     | -    | 1 byte |
| 2      | 2       | 3           | 0           | -    | 1 byte |

### Get Wifi Client Success Response

#### Response Structure

| Header   | Command | Sub-command | Data Length | Success Flag |
| -------- | ------- | ----------- | ----------- | ------------ |
| Response | Network | Wifi Client | 1 byte      | 0 (Success)  |

#### Response Data Parameters

| Parameter             | Values / Format                     | Description                        |
| --------------------- | ----------------------------------- | ---------------------------------- |
| SSID Length           | 1-32 bytes                          | Length of connected network's SSID |
| SSID                  | ASCII string (1-32B)                | Connected network name             |
| Encryption Type       | 1 = Open<br>2 = WPA2<br>3 = WPA3    | Security mode of connected network |
| Encryption Key Length | 1-32 bytes                          | Length of network password         |
| Encryption Key        | ASCII string (1-32B)                | Network password (if available)    |
| IP Address            | IPv4 format (e.g., `192.168.1.123`) | Assigned IP address                |
| Subnet Mask           | IPv4 format (e.g., `255.255.255.0`) | Network subnet mask                |

##### Example Success Response

| Header | Command | Sub-command | Data Length | Success | SSID Len | SSID     | Enc Type | Key Len | Enc Key    | IP Address    | Subnet Mask   | CRC |
| ------ | ------- | ----------- | ----------- | ------- | -------- | -------- | -------- | ------- | ---------- | ------------- | ------------- | --- |
| 4      | 2       | 2           | 5-134       | 0       | 10       | HomeWiFi | 2        | 12      | MyPassword | 192.168.1.123 | 255.255.255.0 | 1   |

###### Notes:
- **Success Flag**: `0` indicates successful response
- **Data Length**: Varies (5-134 bytes) depending on SSID and Key lengths
- **Encryption Types**:
  - `1` = Open (no authentication)
  - `2` = WPA2-PSK
  - `3` = WPA3-SAE
- **IP Configuration**:
  - Shows current assigned IP (DHCP or static)
- **CRC**: 1-byte checksum for data integrity verification

> Note: Encryption Key may be omitted or masked in responses for security reasons

#### Get Wifi Client Error Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | Network | Wifi Client | 1 byte      | Failed              | Error Code | 1 byte |
| 3        | 2       | 3           | 2           | 1                   | -1 to -6   | 1 byte |

### Get Wifi State Request:

| Header | Command | Sub-command | Data Length | Data | CRC |
| ------ | ------- | ----------- | ----------- | ---- | --- |

| Get | Network | Wifi State | 0 bytes | -   | 1 byte |
| --- | ------- | ---------- | ------- | --- | ------ |
| 2   | 2       | 3          | 0       | -   | 1 byte |

#### Get Wifi State Success Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data                   | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------------------- | ------ |
| Response | Network | Wifi State  | 1 byte      | Success             | WifiHotspot/WifiClient | 1 byte |
| 4        | 2       | 3           | 2           | 0                   | 1 / 2                  | 1 byte |

#### Get Wifi State Error Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | Network | Wifi State  | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 2       | 3           | 2           | 1                   | -1 to -6   | 1 byte |




### Get Ethernet Request:

| Header | Command | Sub-command | Data Length | Data | CRC |
| ------ | ------- | ----------- | ----------- | ---- | --- |

| Get | Network | ETHERNET | 0 bytes | -   | 1 byte |
| --- | ------- | -------- | ------- | --- | ------ |
| 2   | 2       | 4        | 0       | -   | 1 byte |

#### Get Ethernet Success Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data        | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ----------- | ------ |
| Response | Network | ETHERNET    | 1 byte      | Success             | IP_ADDRESS  | 1 byte |
| 4        | 2       | 4           | 1-255       | 0                   | 1-255 bytes | 1 byte |

#### Get Ethernet Error Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | Network | ETHERNET    | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 2       | 4           | 1-255       | 1                   | -1 to -6   | 1 byte |


### Get Onvif Interface Request:

| Header | Command | Sub-command | Data Length | Data | CRC |
| ------ | ------- | ----------- | ----------- | ---- | --- |

| Get | Network | onvif interface | 0 bytes | -   | 1 byte |
| --- | ------- | --------------- | ------- | --- | ------ |
| 2   | 2       | 5               | 0       | -   | 1 byte |

#### Get Ethernet Success Response:

| Header   | Command | Sub-command     | Data Length | Success/Failed flag | Data      | CRC    |
| -------- | ------- | --------------- | ----------- | ------------------- | --------- | ------ |
| Response | Network | onvif interface | 1 byte      | Success             | Interface | 1 byte |
| 4        | 2       | 5               | 1-255       | 0                   | 0/1       | 1 byte |

#### Get Ethernet Error Response:

| Header   | Command | Sub-command     | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | --------------- | ----------- | ------------------- | ---------- | ------ |
| Response | Network | onvif interface | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 2       | 5               | 1-255       | 1                   | -1 to -6   | 1 byte |

#### error codes:

| error code | status                                       |
| ---------- | -------------------------------------------- |
| -1         | Invalid Input(invalid onvif interface state) |





### Set ETHERNET Configuration

#### Main Command Structure

| Header | Command | Sub-command | Data Length |
| ------ | ------- | ----------- | ----------- |
| Set    | Network | ETHERNET    | 1 byte      |

#### Data Parameters

| Parameter          | Values / Format                     | Description                        |
| ------------------ | ----------------------------------- | ---------------------------------- |
| IP Address Length  | 1-32 bytes                          | Length of connected network's SSID |
| IP Address         | IPv4 format (e.g., `192.168.1.123`) | *Static IP (if DHCP disabled)*     |
| Subnet Mask Length | 1-32 bytes                          | Length of connected network's SSID |
| Subnet Mask        | IPv4 format (e.g., `255.255.255.0`) | *Required for static IP*           |

##### Example Command

| Header | Command | Sub-command | Data Length | IP Address Length | IP Address    | Subnet Mask Length | Subnet Mask   | CRC |
| ------ | ------- | ----------- | ----------- | ----------------- | ------------- | ------------------ | ------------- | --- |
| 1      | 2       | 4           | 5-133       | 1-32 bytes        | 192.168.1.123 | 1-32 bytes         | 255.255.255.0 | 1   |


- **IP/Subnet Mask**: only for static IP; omit for DHCP.
- **CRC**: Checksum (1 byte).

#### Set ETHERNET Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Network | ETHERNET    | 1 byte      | Success             | Success | 1 byte |
| 3      | 2       | 4           | 2           | 0                   | 0       | 1 byte |

#### Set ETHERNET Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Network | ETHERNET    | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 2       | 4           | 2           | 1                   | -1 to -5   | 1 byte |



### Set ONVIF INTERFACE 

#### Main Command Structure

| Header | Command | Sub-command | Data Length |
| ------ | ------- | ----------- | ----------- |
| Set    | Network | ONVIF       | 1 byte      |

#### Data Parameters

| Parameter       | Values / Format | Description  |
| --------------- | --------------- | ------------ |
| Onvif interface | 0/1             | 0:eth 1:wifi |

##### Example Command

| Header | Command | Sub-command | Data Length | onvif interface | CRC |
| ------ | ------- | ----------- | ----------- | --------------- | --- |
| 1      | 2       | 5           | 1           | 0/1             | 1   |


- **CRC**: Checksum (1 byte).

#### Set ONVIF INTERFACE Success Ack:

| Header | Command | Sub-command     | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | --------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | Network | ONVIF INTERFACE | 1 byte      | Success             | Success | 1 byte |
| 3      | 2       | 5               | 2           | 0                   | 0       | 1 byte |

#### Set ETHERNET Error Ack:

| Header | Command | Sub-command     | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | --------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | Network | ONVIF INTERFACE | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 2       | 5               | 2           | 1                   | -1 to -5   | 1 byte |


#### error codes:

| error code | status                                                            |
| ---------- | ----------------------------------------------------------------- |
| -1         | Invalid Input(invalid onvif interface state)                      |
| -2         | current onvif interface matches with onvif interface tobe changed |
| -3         | not able to restart onvif server                                  |
| -4         | unsupported onvif interface option                                |



# System commands:

## Get Camera name Request:
| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Get    | System  | CAMERA_NAME | 0 bytes     | -    | 1 byte |
| 2      | 6       | 1           | 0           | -    | 1 byte |


#### Get CAMERA_NAME Success Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data        | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ----------- | ------ |
| Response | System  | CAMERA_NAME | 1 byte      | Success             | camera_name | 1 byte |
| 4        | 26      | 1           | 1 - 32      | 0                   | camera_name | 1 byte |


#### Get CAMERA_NAME Error Response:

| Header   | Command | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | firmware_version | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 6       | 1                | 2           | 1                   | -1 to -6   | 1 byte |



## Get firmware_version name Request:
| Header | Command | Sub-command      | Data Length | Data | CRC    |
| ------ | ------- | ---------------- | ----------- | ---- | ------ |
| Get    | System  | firmware_version | 0 bytes     | -    | 1 byte |
| 2      | 6       | 2                | 0           | -    | 1 byte |


#### Get firmware_version Success Response:

| Header   | Command | Sub-command      | Data Length | Success/Failed flag | Data             | CRC    |
| -------- | ------- | ---------------- | ----------- | ------------------- | ---------------- | ------ |
| Response | System  | firmware_version | 1 byte      | Success             | firmware_version | 1 byte |
| 4        | 6       | 2                | 1 - 32      | 0                   | firmware_version | 1 byte |


#### Get firmware_version Error Response:

| Header   | Command | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | firmware_version | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 6       | 2                | 2           | 1                   | -1 to -6   | 1 byte |





## Get mac_address name Request:
| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Get    | System  | mac_address | 0 bytes     | -    | 1 byte |
| 2      | 6       | 3           | 0           | -    | 1 byte |


#### Get mac_address Success Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data        | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ----------- | ------ |
| Response | System  | mac_address | 1 byte      | Success             | mac_address | 1 byte |
| 4        | 6       | 3           | 1 - 32      | 0                   | mac_address | 1 byte |


#### Get mac_address Error Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | mac_address | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 6       | 3           | 2           | 1                   | -1 to -6   | 1 byte |



## login_pin  Request:
| Header | Command | Sub-command | Data Length | Data      | CRC    |
| ------ | ------- | ----------- | ----------- | --------- | ------ |
| Get    | System  | login_pin   | 1 byte      | login_pin | 1 byte |
| 2      | 6       | 4           | 1-32        | -         | 1 byte |


####  login_pin Success Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data      | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | --------- | ------ |
| Response | System  | login_pin   | 1 byte      | Success             | login_pin | 1 byte |
| 4        | 6       | 4           | 1 - 32      | 0                   | login_pin | 1 byte |


####  login_pin Error Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | login_pin   | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 6       | 4           | 2           | 1                   | -1 to -6   | 1 byte |





## Get ota_status name Request:
| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Get    | System  | ota_status  | 0 bytes     | -    | 1 byte |
| 2      | 6       | 4           | 0           | -    | 1 byte |


#### Get ota_status Success Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | ota_status  | 1 byte      | Success             | ota_status | 1 byte |
| 4        | 6       | 4           | 1 - 32      | 0                   | ota_status | 1 byte |


#### Get ota_status Error Response:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | ota_status  | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 6       | 4           | 2           | 1                   | -1 to -6   | 1 byte |





## Get  camera health_check Request:
| Header | Command | Sub-command  | Data Length | Data | CRC    |
| ------ | ------- | ------------ | ----------- | ---- | ------ |
| Get    | System  | health_check | 0 bytes     | -    | 1 byte |
| 2      | 6       | 6            | 0           | -    | 1 byte |


#### Get camera health_check Success Response:

| Header   | Command | Sub-command  | Data Length | Success/Failed flag | Data     |
| -------- | ------- | ------------ | ----------- | ------------------- | -------- |
| Response | System  | health_check | 1 byte      | Success             | streamer |
| 4        | 6       | 6            | 1 - 32      | 0                   | 0/1      |


| Data  |             |           |              |                 |                |                    | CRC    |
| ----- | ----------- | --------- | ------------ | --------------- | -------------- | ------------------ | ------ |
| rtsps | portablertc | cpu_usage | memory_usage | isp_temparature | ir_temparature | sensor_temparature | 1 byte |
| 0/1   | 0/1         | 0-100     | 0-100        | 0-100           | 0-100          | 0-100              | 1 byte |




#### Get camera health_check Error Response:

| Header   | Command | Sub-command  | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ------------ | ----------- | ------------------- | ---------- | ------ |
| Response | System  | health_check | 1 byte      | Failed              | Error Code | 1 byte |
| 4        | 6       | 6            | 2           | 1                   | -1 to -6   | 1 byte |





## Set Camera name Request:
| Header | Command | Sub-command | Data Length | Data        | CRC    |
| ------ | ------- | ----------- | ----------- | ----------- | ------ |
| Set    | System  | CAMERA_NAME | 1-32 bytes  | camera_name | 1 byte |
| 1      | 6       | 1           | 1-32        | camera_name | byte   |


#### Set CAMERA_NAME Success ACK:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data        | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | ----------- | ------ |
| Response | System  | CAMERA_NAME | 1 byte      | Success             | camera_name | 1 byte |
| 3        | 6       | 1           | 1 - 32      | 0                   | camera_name | 1 byte |


#### Set CAMERA_NAME Error ACK:

| Header   | Command | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | firmware_version | 1 byte      | Failed              | Error Code | 1 byte |
| 3        | 6       | 1                | 2           | 1                   | -1 to -6   | 1 byte |




## Set login_pin Request:
| Header | Command | Sub-command | Data Length | Data      | CRC    |
| ------ | ------- | ----------- | ----------- | --------- | ------ |
| Set    | System  | login_pin   | 1-32 bytes  | login_pin | 1 byte |
| 1      | 6       | 2           | 1-32        | login_pin | byte   |


#### Set login_pin Success ACK:

| Header   | Command | Sub-command | Data Length | Success/Failed flag | Data      | CRC    |
| -------- | ------- | ----------- | ----------- | ------------------- | --------- | ------ |
| Response | System  | login_pin   | 1 byte      | Success             | login_pin | 1 byte |
| 3        | 6       | 2           | 1 - 32      | 0                   | login_pin | 1 byte |


#### Set login_pin Error ACK:

| Header   | Command | Sub-command      | Data Length | Success/Failed flag | Data       | CRC    |
| -------- | ------- | ---------------- | ----------- | ------------------- | ---------- | ------ |
| Response | System  | firmware_version | 1 byte      | Failed              | Error Code | 1 byte |
| 3        | 6       | 2                | 2           | 1                   | -1 to -6   | 1 byte |






## Factory Reset:

| Header | Command | Sub-command   | Data Length | Data | CRC    |
| ------ | ------- | ------------- | ----------- | ---- | ------ |
| Set    | System  | Factory Reset | 0 bytes     | -    | 1 byte |
| 1      | 6       | 3             | 0           | -    | 1 byte |

## Factory Reset Success Ack:

| Header | Command | Sub-command   | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ------------- | ----------- | ------------------- | ------- | ------ |
| Ack    | System  | Factory Reset | 1 byte      | Success             | Success | 1 byte |
| 3      | 6       | 3             | 2           | 0                   | 0       | 1 byte |

## Factory Reset Error Ack:

| Header | Command | Sub-command   | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ------------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | System  | Factory Reset | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 6       | 3             | 2           | 1                   | -1 to -6   | 1 byte |




## Shutdown:

| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Set    | System  | SHUTDOWN    | 0 bytes     | -    | 1 byte |
| 1      | 6       | 4           | 0           | -    | 1 byte |

## Shutdown Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | System  | SHUTDOWN    | 1 byte      | Success             | Success | 1 byte |
| 3      | 6       | 4           | 2           | 0                   | 0       | 1 byte |

## Start Streaming Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | System  | SHUTDOWN    | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 6       | 4           | 2           | 1                   | -1 to -6   | 1 byte |



## OTA-Update:

| Header | Command | Sub-command | Data Length | Data | CRC    |
| ------ | ------- | ----------- | ----------- | ---- | ------ |
| Set    | System  | OTA update  | 0 bytes     | -    | 1 byte |
| 1      | 6       | 5           | 0           | -    | 1 byte |

## Shutdown Success Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data    | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ------- | ------ |
| Ack    | System  | OTA update  | 1 byte      | Success             | Success | 1 byte |
| 3      | 6       | 5           | 2           | 0                   | 0       | 1 byte |

## Start Streaming Error Ack:

| Header | Command | Sub-command | Data Length | Success/Failed flag | Data       | CRC    |
| ------ | ------- | ----------- | ----------- | ------------------- | ---------- | ------ |
| Ack    | System  | OTA update  | 1 byte      | Failed              | Error Code | 1 byte |
| 3      | 6       | 5           | 2           | 1                   | -1 to -6   | 1 byte |



### Set Provision Mode : set macaddress, serial_number,manufacturing date

#### Main Command Structure

| Header | Command | Sub-command    | Data Length |
| ------ | ------- | -------------- | ----------- |
| Set    | System  | Provision mode | 1 byte      |

#### Data Parameters

| Parameter            | Values / Format      | Description                    |
| -------------------- | -------------------- | ------------------------------ |
| MACID Length         | 1-32 bytes           | Length of the MACID string     |
| MACID                | ASCII string (1-32B) | Mac ADDRESS                    |
| Serial Number Len    | 1 -32                | Length of the Serial string    |
| Serial Number        | 1-32 bytes           | SERIAL NUMBER                  |
| Manufacture Date Len | 1-32                 | Length of the Manufacture Date |
| Manufacture Date     | 1-32 bytes           | YYYY/MM/DD                     |



##### Example Command

| Header | Command | Sub-command | Data Length | MacID Len | MACID             | Serial Number Len | Serial Number | Manufacture Date Len | Manufacture Date | CRC |
| ------ | ------- | ----------- | ----------- | --------- | ----------------- | ----------------- | ------------- | -------------------- | ---------------- | --- |
| 1      | 2       | 1           | 5-133       | 1-32      | 88:ae:dd:65:ff:4c | 8                 | mypass123     | 11                   | yyyy/mm/dd       | 1   |










# Multiple Resolutions Support

# Sample commands:

## Ex 1: Set Mirror Value:

Create byte array packet as follows:
(In C): `unsigned char packet[] = {1, 4, 7, 1, 1, 242};`

- 1st byte 1 is Set Header
- 2nd byte 4 is Image Command
- 3rd byte 7 is Mirror Sub command
- 4th byte 1 is DataLength
- 5th byte 1 is Data (Mirror value On)
- 6th byte 242 is CRC (2's complement of (1+4+7+1+1))

Send the packet to the camera, using socket communication.

## Ex 2: Set WifiClient:

Create byte array packet as follows:
(In C): `unsigned char packet[] = {1, 2, 1, 39, 4, 116(t), 101(e), 115(s), 116(t), 3, 4, 116(t), 101(e), 115(s), 116(t), 13, 49(1), 57(9), 50(2), 46(.), 49(1), 54(6), 56(8), 46(.), 49(1), 46(.), 49(1), 50(2), 51(3), 13, 50(2), 53(5), 53(5), 46(.), 50(2), 53(5), 53(5), 46(.), 50(2), 53(5), 53(5), 46(.), 48(0), 22};`

- 1st byte 1 is Set Header
- 2nd byte 2 is Network Command
- 3rd byte 2 is Wifi Client Sub command
- 4th byte 39 is DataLength
- 5th byte 4 is SSID Length
- 6 to 9 bytes are SSID name (test)
- 10th byte 3 is Encryption Type WPA2
- 11th byte 4 is Encryption Key Len
- 12 to 15 bytes are Encryption Key (test)
- 16th byte 13 is IpAddress Len
- 17 to 29 bytes are IpAddress 192.168.1.123
- 30th byte 13 is Subnetmask Len
- 31 to 43 bytes are Subnetmask 255.255.255.0
- 44th byte 22 is CRC (2's complement of (1+2+1+39+4+116+101+115+116+3+4+116+101+115+116+13+49+57+50+46+49+54+56+46+49+46+49+50+51+13+50+53+53+46+50+53+53+46+50+53+53+46+48))

Send the packet to the camera, using socket communication.

## Ex 3: Start Streaming:

Create byte array packet as follows:
(In C): `unsigned char packet[] = {1, 1, 1, 0, 5};`

- 1st byte 1 is Set Header
- 2nd byte 1 is Streaming Command
- 3rd byte 1 is Start Sub command
- 4th byte 0 is DataLength
- 5th byte 253 is CRC (2's complement of (1+1+1+0))

Send the packet to the camera, using socket communication.

## FAQ

**What is the use case for haptic?**
It will happen automatically on camera start.

**How to calculate CRC?**
1. Step 1: Add all the packet bytes (except CRC byte)
2. Step 2: 2's Complement the above sum
3. Step 3: Take lower 8 bits as CRC

**How to validate CRC?**
1. Step 1: Add all the packet bytes (including CRC byte)
2. Step 2: Lower 8 bits should be zero.

**How to connect to the camera the first time, and the process for configuring the Wi-Fi?**
Camera acts as a Wi-Fi hotspot with default SSID, encryption type & key, IP address.

- Default SSID: nveyetech<IPAddress last octet-150> (ex: if Ipaddress is 192.168.1.183, then 183-150=33, and SSID is nveyetech33)
- Default encryption type: WPA2 + TKIP/AES
- Default encryption key: 1234567890
- Default IP address: 192.168.2.1
- Default Subnet mask: 255.255.255.0
