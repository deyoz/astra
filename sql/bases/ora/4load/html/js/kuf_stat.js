function dateFormat()
{
    return 'dd.mm.y';
}

function downloadError()
{
    alert('file download failed');
}

function downloadFile(data, filename, type) {
    var a = document.createElement("a"),
    file = new Blob([data], {type: type});
    if (window.navigator.msSaveOrOpenBlob) // IE10+
        window.navigator.msSaveOrOpenBlob(file, filename);
    else { // Others
        var url = URL.createObjectURL(file);
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        setTimeout(function() {
                document.body.removeChild(a);
                window.URL.revokeObjectURL(url);  
                }, 0); 
    }
}

function downloadSuccess(res) {
    var xmlDoc = $.parseXML(res);
    if(handleServerError(xmlDoc)) {
        downloadFile(
                Base64.decode($(xmlDoc).find('data').text()),
                $(xmlDoc).find('name').text(),
                "text/xml");
    }
}

function download()
{
    $.ajax({
        url: this.href,
        type: 'get',
        success: downloadSuccess,
        error: downloadError
    });
    return false;
}

function createTable(xml)
{
    var node = $(xml).find('item');

    var dvTable = $(".dvTable");
    dvTable.html("");

    if(!node.length) {
        dvTable.append("<h2>Нет данных на указанную дату.</h2>");
        return;
    }

    //Create a HTML Table element.
    var table = $("<table/>");
    table[0].border = "1";

    //Add the header row.
    var row = $(table[0].insertRow(-1));

    row.append('<th rowspan=2>Рейс</th>');
    row.append('<th rowspan=2>Статус</th>');
    row.append('<th colspan=2>Общие данные</th>');
    row.append('<th rowspan=2>Список пассажиров</th>');
    row = $(table[0].insertRow());
    row.append('<th >Закрытие регистрации</th>');
    row.append('<th >Вылет</th>');

//    row.append("<th>Рейс</th><th>Статус</th><th>Общие данные</th><th>Список пассажиров</th>");

    //Add the data rows.
    $(node).each(function () {
            row = $(table[0].insertRow(-1));
            var aflt_status;
            var apoint_id;
            $(this).children().each(function (index) {
                if(this.nodeName == 'flt') {
                    aflt_status = $(this).attr('flt_status');
                    apoint_id = $(this).attr('point_id');
                }
                if(
                    this.nodeName == 'cc' ||
                    this.nodeName == 'pax' ||
                    this.nodeName == 'cl'
                    ) {
                    var params = {
                        OPERATION: 'kuf_stat',
                        point_id: apoint_id,
                        flt_status: aflt_status,
                        file_type: this.nodeName
                    };
                    var matches = /(.*)\/.*$/.exec(window.location.pathname);
                    row.append("<td id='data'><a href='" + matches[1] + "?" + $.param(params) + "'>" + $(this).text() + "</a></td>");
                } else
                    row.append("<td id='data'>" + $(this).text() + "</td>");
                });
            });

    dvTable.append(table);
    $('a').click(download);
}

function handleServerError(xmlDoc)
{
    var user_error = $(xmlDoc).find("user_error");
    if($(user_error).text() !== '') {
        alert($(user_error).text());
        return false;
    } else {
        $term = $(xmlDoc).find("term");
        if($term.text() !== '') {
            alert("Ошибка севера. Обратитесь к разработчикам");
            return false;
        }
    }
    return true;
}

function ajaxSuccess(res)
{
    var xmlDoc = $.parseXML(res);
    if(handleServerError(xmlDoc)) createTable(xmlDoc);
}

function ajaxError()
{
    alert('ajax error');
}

function btnClick()
{
    var d;
    try {
        d = $.datepicker.parseDate(dateFormat(), $('input').val());
    } catch(e) {
        alert('Неверный формат даты');
        return;
    }
    if($(this).attr('id')) {
        var date = new Date( Date.parse(d) );
        var inc = $(this).attr('id') == 'back' ? -1 : 1;
        date.setDate( date.getDate() + inc );
        $('input').val($.datepicker.formatDate(dateFormat(), date));
    }

    $.ajax({
        type: 'get',
        data: {
            OPERATION: 'kuf_stat_flts',
            scd_out: $('input').val()
        },
        success: ajaxSuccess,
        error: ajaxError
    });

}

function inputKeyPress(e)
{
    if(e.keyCode == 13) btnClick();
}

$(function() {
        $('input').val($.datepicker.formatDate(dateFormat(), new Date()));
        $('button').click(btnClick);
        $('input').keypress(inputKeyPress);
        btnClick();
    }
);

var Base64 = {

    // private property
    _keyStr : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",

    // public method for encoding
    encode : function (input) {
        var output = "";
        var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
        var i = 0;

        input = Base64._utf8_encode(input);

        while (i < input.length) {

            chr1 = input.charCodeAt(i++);
            chr2 = input.charCodeAt(i++);
            chr3 = input.charCodeAt(i++);

            enc1 = chr1 >> 2;
            enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
            enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
            enc4 = chr3 & 63;

            if (isNaN(chr2)) {
                enc3 = enc4 = 64;
            } else if (isNaN(chr3)) {
                enc4 = 64;
            }

            output = output +
            this._keyStr.charAt(enc1) + this._keyStr.charAt(enc2) +
            this._keyStr.charAt(enc3) + this._keyStr.charAt(enc4);

        }

        return output;
    },

    // public method for decoding
    decode : function (input) {
        var output = "";
        var chr1, chr2, chr3;
        var enc1, enc2, enc3, enc4;
        var i = 0;

        input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");

        while (i < input.length) {

            enc1 = this._keyStr.indexOf(input.charAt(i++));
            enc2 = this._keyStr.indexOf(input.charAt(i++));
            enc3 = this._keyStr.indexOf(input.charAt(i++));
            enc4 = this._keyStr.indexOf(input.charAt(i++));

            chr1 = (enc1 << 2) | (enc2 >> 4);
            chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
            chr3 = ((enc3 & 3) << 6) | enc4;

            output = output + String.fromCharCode(chr1);

            if (enc3 != 64) {
                output = output + String.fromCharCode(chr2);
            }
            if (enc4 != 64) {
                output = output + String.fromCharCode(chr3);
            }

        }

        output = Base64._utf8_decode(output);

        return output;

    },

    // private method for UTF-8 encoding
    _utf8_encode : function (string) {
        string = string.replace(/\r\n/g,"\n");
        var utftext = "";

        for (var n = 0; n < string.length; n++) {

            var c = string.charCodeAt(n);

            if (c < 128) {
                utftext += String.fromCharCode(c);
            }
            else if((c > 127) && (c < 2048)) {
                utftext += String.fromCharCode((c >> 6) | 192);
                utftext += String.fromCharCode((c & 63) | 128);
            }
            else {
                utftext += String.fromCharCode((c >> 12) | 224);
                utftext += String.fromCharCode(((c >> 6) & 63) | 128);
                utftext += String.fromCharCode((c & 63) | 128);
            }

        }

        return utftext;
    },

    // private method for UTF-8 decoding
    _utf8_decode : function (utftext) {
        var string = "";
        var i = 0;
        var c = c1 = c2 = 0;

        while ( i < utftext.length ) {

            c = utftext.charCodeAt(i);

            if (c < 128) {
                string += String.fromCharCode(c);
                i++;
            }
            else if((c > 191) && (c < 224)) {
                c2 = utftext.charCodeAt(i+1);
                string += String.fromCharCode(((c & 31) << 6) | (c2 & 63));
                i += 2;
            }
            else {
                c2 = utftext.charCodeAt(i+1);
                c3 = utftext.charCodeAt(i+2);
                string += String.fromCharCode(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
                i += 3;
            }

        }

        return string;
    }

}
