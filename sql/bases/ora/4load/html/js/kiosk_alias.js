function createTable(xml)
{
    var node = $(xml).find('item');

    var dvTable = $(".dvTable");
    dvTable.html("");

    if(!node.length) {
        dvTable.append("<h2>Нет данных</h2>");
        return;
    }

    /*
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
    */
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
    $.ajax({
        type: 'get',
        data: {
            OPERATION: 'kiosk_alias',
        },
        success: ajaxSuccess,
        error: ajaxError
    });
}

$(function() {
        $('button').click(btnClick);
        btnClick();
    }
);
