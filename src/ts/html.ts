include(ts/macro.ts)

# meta: suite html

$(init)

!! capture=on req_type=http
$(http_wrap
{GET /web_srv.html?CLIENT-ID=HTML&login=HTML&password=HTMLPWD HTTP/1.1
Host: astrabeta.komtex:8782
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.60 YaBrowser/20.12.0.963 Yowser/2.5 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
Accept-Encoding: gzip, deflate
Accept-Language: ru,en;q=0.9
$()})

>> lines=auto
$(http_wrap
{HTTP/1.1 200 OK
Content-Length: 4604
Content-Type: $()
Access-Control-Allow-Origin: *
Access-Control-Allow-Headers: CLIENT-ID,OPERATION,Authorization
Cache-Control: no-cache
ETag: 430245ce720b3fc1dea98dc11f91aea0
Last-Modified: Wed, 30 Dec 2020 09:12:02 GMT
$()
<!doctype html>
<html>
<head>
<meta http-equiv='Content-Type' charset='UTF-8' />
<script src='js/jquery-ui-1.12.1.custom/external/jquery/jquery.js'></script>
<script src='js/web_srv.js' type='text/javascript'></script>
<script src='js/requests.js' type='text/javascript'></script>
<script src='js/connections.js' type='text/javascript'></script>
<link href='css/web_srv.css' type='text/css' rel='stylesheet'>
<title>Веб-сервис DCS Астра</title>
</head>
<body>
<center>
<h1>Веб-сервис DCS Астра</h1>
<form action='https://astra-wst.sirena-travel.ru/astra-test/' method='post' onsubmit='AJAXSubmit(this); return false;'>
    <table>
        <tr>
            <td>
                <div class='currentmasterreport'>
                    <table sellpadding=0 cellspacing=0 scrollable=true scrollHeight=50>
                        <tr>
                            <th>Список запросов</th>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 0)' class=currentrow>
                            <td class=currentcell>Запрос весов паксов</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 1)'>
                            <td>Установка весов паксов</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 2)'>
                            <td>LCI запрос</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 3)'>
                            <td>Установка макс. комм. загр.</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 4)'>
                            <td>Контроль компоновок. Конфиг. салона.</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 5)'>
                            <td>Контроль компоновок. Список мест.</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 6)'>
                            <td>Теги для пос. талонов</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 7)'>
                            <td>WBW СЗВ</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 8)'>
                            <td>run_stat</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 9)'>
                            <td>CREWCHECKIN</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 10)'>
                            <td>Печать ПТ</td>
                        </tr>
                        <tr onclick='table_onSelectMasterRow(this, 0, 11)'>
                            <td>NTM</td>
                        </tr>
                    </table>
                </div>
            </td>
            <td class='dest_cell'>
              <fieldset>
                <legend>Адресат</legend>
                <select id='destination' onChange='select_change()'></select>
                <p>
                <table>
                  <tr>
                    <td>Логин</td>
                    <td id='dst_login'></td>
                  </tr>
                  <tr>
                    <td>Пароль</td>
                    <td id='dst_pwd'></td>
                  </tr>
                  <tr>
                    <td>Клиент</td>
                    <td id='dst_client'></td>
                  </tr>
                  <tr>
                    <td>Сервер</td>
                    <td id='dst_addr'></td>
                  </tr>
                </table>
              </fieldset>
            </td>
        </tr>
        <tr>
            <td>
                Запрос:<br />
                <textarea id='content' name='description' cols='50' rows='8'></textarea>
            </td>
            <td>
                Ответ:<br />
                <textarea id='response' name='response' cols='50' rows='8'></textarea>
            </td>
        </tr>
        <tr>
            <td>
                <input type='submit' value='Отправить' />
            </td>
        </tr>
    </table>
</form>
</center>
</body>
</html>})
