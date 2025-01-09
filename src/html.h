const char CSS_styles[] = R"css(
<style>
    html,
    body,
    div,
    input {
        margin: 0;
        padding: 0;
        border: 0;
        font-family: 'Itim', sans-serif, Arial;
    }

    html,
    body {
        min-height: 100%;
        overflow-x: hidden;
    }

    body {
        background: #2c3e50;
        background: -webkit-linear-gradient(315deg, hsla(236.6, 0%, 53.52%, 1) 0, hsla(236.6, 0%, 53.52%, 0) 70%),
            -webkit-linear-gradient(65deg, hsla(220.75, 34.93%, 26.52%, 1) 10%, hsla(220.75, 34.93%, 26.52%, 0) 80%),
            -webkit-linear-gradient(135deg, hsla(46.42, 36.62%, 83.92%, 1) 15%, hsla(46.42, 36.62%, 83.92%, 0) 80%),
            -webkit-linear-gradient(205deg, hsla(191.32, 50.68%, 56.45%, 1) 100%, hsla(191.32, 50.68%, 56.45%, 0) 70%);
        background: linear-gradient(135deg, hsla(236.6, 0%, 53.52%, 1) 0, hsla(236.6, 0%, 53.52%, 0) 70%),
            linear-gradient(25deg, hsla(220.75, 34.93%, 26.52%, 1) 10%, hsla(220.75, 34.93%, 26.52%, 0) 80%),
            linear-gradient(315deg, hsla(46.42, 36.62%, 83.92%, 1) 15%, hsla(46.42, 36.62%, 83.92%, 0) 80%),
            linear-gradient(245deg, hsla(191.32, 50.68%, 56.45%, 1) 100%, hsla(191.32, 50.68%, 56.45%, 0) 70%);
    }

    .fixMinHeight {
        display: -webkit-box;
        display: -ms-flexbox;
        display: flex;
    }

    .main {
        min-height: calc(100vh - 90px);
        width: 100%;
        display: -webkit-box;
        display: -ms-flexbox;
        display: flex;
        -webkit-box-orient: vertical;
        -webkit-box-direction: normal;
        -ms-flex-direction: column;
        flex-direction: column;
    }

    .wrap {
        margin: auto;
        padding: 40px;
        -webkit-transition: width .3s ease-in-out;
        transition: width .3s ease-in-out;
        background: #00000040;
        border-radius: 6px;
    }

    @media only screen and (min-width:1px) and (max-width:575px) {
        .main {
            min-height: calc(100vh);
        }

        .wrap {
            width: 100%;
            height: 100%;
            border-radius: 0px;
        }
    }

    @media (min-width:576px) {
        .wrap {
            width: 400px;
        }

        * {
            font-size: 14px !important;
        }
    }

    * {
        -webkit-box-sizing: border-box;
        box-sizing: border-box;
        font-size: 16px;
        color: #fff;
        text-shadow: 1px 1px 2px #00000080;
    }

    table {
        border-collapse: collapse;
        width: 100%;
    }

    table td {
        padding: 10px;
        border-bottom: 1px solid #ffffff30;
    }

    table td:nth-child(2) {
        text-align: right;
    }

    h1 {
        text-align: center;
        font-size: 24px !important;
        margin-top: 5px;
    }

    label {
        display: block;
        margin-bottom: 10px;
        position: relative;
    }

    input[type="text"],
    input[type="password"] {
        color: #33404a;
        width: 100%;
        height: 44px;
        padding: 3px 20px;
        margin-bottom: 20px;
        border: 1px solid #ccc;
        border-radius: 6px;
        box-sizing: border-box;
        text-shadow: none;
    }

    input[type=submit] {
        background: #3498db;
        color: #fff;
        border: 0;
        cursor: pointer;
        text-align: center;
        width: 100%;
        height: 44px;
        border-radius: 6px;
        margin-bottom: 10px;
        -webkit-transition: background .3s ease-in-out;
        transition: background .3s ease-in-out;
    }

    input[type="submit"]:hover {
        background: #2980b9;
    }

    .ip-config {
        margin-bottom: 20px;
    }

    .ip-config label,
    .ip-config input {
        display: inline-block;
        vertical-align: middle;
    }

    .ip-config input[type="radio"] {
        margin-left: 10px;
    }

    #staticIPConfig {
        display: none;
    }
</style>
)css";


const char MAIN_page[] = R"html(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta http-equiv="pragma" content="no-cache">
    <meta http-equiv="expires" content="-1">
    <title>Solar Monitor</title>
    <link rel="icon" type="image/png"
        href="https://cdn.icon-icons.com/icons2/2836/PNG/512/station_solar_icon_180267.png">
    <link href="https://fonts.googleapis.com/css2?family=Itim&display=swap" rel="stylesheet">
    %CSS_STYLES%
</head>

<body>
    <div class="fixMinHeight">
        <div class="main">
            <div class="wrap">
                <table>
                    <tbody>
                        <tr>
                            <td>PV Voltage</td>
                            <td><span id="PV_Voltage"></span> V</td>
                        </tr>
                        <tr>
                            <td>PV Current</td>
                            <td><span id="PV_Current"></span> A</td>
                        </tr>
                        <tr>
                            <td>PV Power</td>
                            <td><span id="PV_Power"></span> kW</td>
                        </tr>
                        <tr>
                            <td>Bus Voltage</td>
                            <td><span id="Bus_Voltage"></span> V</td>
                        </tr>
                        <tr>
                            <td>AC Voltage</td>
                            <td><span id="AC_Voltage"></span> V</td>
                        </tr>
                        <tr>
                            <td>AC Current</td>
                            <td><span id="AC_Current"></span> A</td>
                        </tr>
                        <tr>
                            <td>Grid Frequency</td>
                            <td><span id="Grid_Frequency"></span> Hz</td>
                        </tr>
                        <tr>
                            <td>Active Power</td>
                            <td><span id="Active_Power"></span> kW</td>
                        </tr>
                        <tr>
                            <td>Reactive Power</td>
                            <td><span id="Reactive_Power"></span> kVar</td>
                        </tr>
                        <tr>
                            <td>Daily Production</td>
                            <td><span id="Daily_Production"></span> kWh</td>
                        </tr>
                        <tr>
                            <td>Total Production</td>
                            <td><span id="Total_Production"></span> kWh</td>
                        </tr>
                        <tr>
                            <td>Temperature Module</td>
                            <td><span id="Temperature_Module"></span> °C</td>
                        </tr>
                        <tr>
                            <td>Temperature Inverter</td>
                            <td><span id="Temperature_Inverter"></span> °C</td>
                        </tr>
                        <tr>
                            <td>Total Running Hour</td>
                            <td><span id="Total_Running_Hour"></span> h</td>
                        </tr>
                        <tr>
                            <td>Wi-Fi Signal Strength</td>
                            <td><span id="RSSI"></span> dBm</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
    <script>
        function fetchData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('PV_Voltage').innerText = data.PV_Voltage || '0';
                    document.getElementById('PV_Current').innerText = data.PV_Current || '0';
                    document.getElementById('PV_Power').innerText = data.PV_Power || '0';
                    document.getElementById('Bus_Voltage').innerText = data.Bus_Voltage || '0';
                    document.getElementById('AC_Voltage').innerText = data.AC_Voltage || '0';
                    document.getElementById('AC_Current').innerText = data.AC_Current || '0';
                    document.getElementById('Grid_Frequency').innerText = data.Grid_Frequency || '0';
                    document.getElementById('Active_Power').innerText = data.Active_Power || '0';
                    document.getElementById('Reactive_Power').innerText = data.Reactive_Power || '0';
                    document.getElementById('Daily_Production').innerText = data.Daily_Production || '0';
                    document.getElementById('Total_Production').innerText = data.Total_Production ? data.Total_Production.toLocaleString() : '0';
                    document.getElementById('Temperature_Module').innerText = data.Temperature_Module || '0';
                    document.getElementById('Temperature_Inverter').innerText = data.Temperature_Inverter || '0';
                    document.getElementById('Total_Running_Hour').innerText = data.Total_Running_Hour ? data.Total_Running_Hour.toLocaleString() : '0';
                    document.getElementById('RSSI').innerText = data.RSSI;
                });
        }
        setInterval(fetchData, 2000);
    </script>
</body>

</html>
)html";


const char WiFi_page[] = R"html(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta http-equiv="pragma" content="no-cache">
    <meta http-equiv="expires" content="-1">
    <title>WiFi Configuration</title>
    %CSS_STYLES%
</head>

<body>
    <div class="fixMinHeight">
        <div class="main">
            <div class="wrap">
                <h1>WiFi Configuration</h1>
                <form action="/save" method="post">
                    <label for="ssid">SSID</label>
                    <input id="ssid" name="ssid" type="text" placeholder="SSID" required />
                    <label for="password">Password</label>
                    <input id="password" name="password" type="password" placeholder="Password" />
                    <div id="staticIPConfig">
                        <label for="address">Address</label>
                        <input id="address" name="address" type="text" placeholder="Address" />
                        <label for="gateway">Gateway</label>
                        <input id="gateway" name="gateway" type="text" placeholder="Gateway" />
                        <label for="netmask">Netmask</label>
                        <input id="netmask" name="netmask" type="text" placeholder="Netmask" />
                    </div>
                    <div class="ip-config">
                        <a>IP Configuration :</a>
                        <input type="radio" id="dynamic" name="ipconfig" value="dynamic" checked>
                        <label for="dynamic">Auto</label>
                        <input type="radio" id="static" name="ipconfig" value="static">
                        <label for="static">Static</label>
                    </div>
                    <input type="submit" value="Connect" />
                </form>
            </div>
        </div>
    </div>
    <script>
        function toggleIPConfig() {
            const staticIPConfig = document.getElementById('staticIPConfig');
            staticIPConfig.style.display = 'none';
            document.querySelectorAll('input[name="ipconfig"]').forEach(elem => {
                elem.addEventListener('change', function () {
                    if (this.value === 'static') {
                        staticIPConfig.style.display = 'block';
                        staticIPConfig.querySelectorAll('input').forEach(input => input.required = true);
                    } else {
                        staticIPConfig.style.display = 'none';
                        staticIPConfig.querySelectorAll('input').forEach(input => input.required = false);
                    }
                });
            });
        }
        document.addEventListener('DOMContentLoaded', toggleIPConfig);
    </script>
</body>

</html>
)html";