const char MAIN_page[] = R"html(
<!DOCTYPE html>
<html>

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta http-equiv="pragma" content="no-cache">
    <meta http-equiv="expires" content="-1">
    <title>Solar Monitor</title>
    <link rel="icon" type="image/png"
        href="https://cdn.icon-icons.com/icons2/2836/PNG/512/station_solar_icon_180267.png">
    <link href="https://fonts.googleapis.com/css2?family=Itim&display=swap" rel="stylesheet">
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
            background: #555555;
            background: -webkit-linear-gradient(315deg, hsla(236.6, 0%, 53.52%, 1) 0, hsla(236.6, 0%, 53.52%, 0) 70%),
                -webkit-linear-gradient(65deg, hsla(220.75, 34.93%, 26.52%, 1) 10%, hsla(220.75, 34.93%, 26.52%, 0) 80%),
                -webkit-linear-gradient(135deg, hsla(46.42, 36.62%, 83.92%, 1) 15%, hsla(46.42, 36.62%, 83.92%, 0) 80%),
                -webkit-linear-gradient(205deg, hsla(191.32, 50.68%, 56.45%, 1) 100%, hsla(191.32, 50.68%, 56.45%, 0) 70%);
            background: linear-gradient(135deg, hsla(236.6, 0%, 53.52%, 1) 0, hsla(236.6, 0%, 53.52%, 0) 70%),
                linear-gradient(25deg, hsla(220.75, 34.93%, 26.52%, 1) 10%, hsla(220.75, 34.93%, 26.52%, 0) 80%),
                linear-gradient(315deg, hsla(46.42, 36.62%, 83.92%, 1) 15%, hsla(46.42, 36.62%, 83.92%, 0) 80%),
                linear-gradient(245deg, hsla(191.32, 50.68%, 56.45%, 1) 100%, hsla(191.32, 50.68%, 56.45%, 0) 70%);
        }

        .ie-fixMinHeight {
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
            background: #00000030;
            border-radius: 10px;
            box-shadow: 2px 3px 6px #00000060;
        }

        @media only screen and (min-width:1px) and (max-width:575px) {
            .main {
                min-height: calc(100vh);
            }

            .wrap {
                width: 100%;
                height: 100%;
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
            color: #ffffff;
            text-shadow: 1px 1px 3px #00000080;
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
    </style>
</head>

<body>
    <div class="ie-fixMinHeight">
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
                            <td><span id="Reactive_Power"></span> VAR</td>
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
                    document.getElementById('PV_Voltage').innerText = data.PV_Voltage;
                    document.getElementById('PV_Current').innerText = data.PV_Current;
                    document.getElementById('PV_Power').innerText = data.PV_Power;
                    document.getElementById('Bus_Voltage').innerText = data.Bus_Voltage;
                    document.getElementById('AC_Voltage').innerText = data.AC_Voltage;
                    document.getElementById('AC_Current').innerText = data.AC_Current;
                    document.getElementById('Grid_Frequency').innerText = data.Grid_Frequency;
                    document.getElementById('Active_Power').innerText = data.Active_Power;
                    document.getElementById('Reactive_Power').innerText = data.Reactive_Power;
                    document.getElementById('Daily_Production').innerText = data.Daily_Production;
                    document.getElementById('Total_Production').innerText = data.Total_Production.toLocaleString();
                    document.getElementById('Temperature_Module').innerText = data.Temperature_Module;
                    document.getElementById('Temperature_Inverter').innerText = data.Temperature_Inverter;
                    document.getElementById('Total_Running_Hour').innerText = data.Total_Running_Hour.toLocaleString();
                    document.getElementById('RSSI').innerText = data.RSSI;
                });
        }
        setInterval(fetchData, 3000);
    </script>
</body>

</html>
)html";