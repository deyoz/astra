"use strict";

function ajaxSuccess () {
//  console.log(this.responseText);
  document.getElementById('response').value =
      (this.responseText == '' ?
       "HTTP status: " +
       this.status + " " + this.statusText : this.responseText);
}

function AJAXSubmit (oFormElement) {
  if (!oFormElement.action) { return; }
  var oReq = new XMLHttpRequest();
  oReq.onload = ajaxSuccess;
  if (oFormElement.method.toLowerCase() === "post") {
    var content = document.getElementById('content').value;
    oReq.open("post", oFormElement.action);
    oReq.setRequestHeader ("Authorization", "Basic " + btoa(get_username() + ":" + get_password()));
    oReq.setRequestHeader ("CLIENT-ID", get_client_id());
    var rowindex = $(".currentrow").index();
    switch(rowindex) {
        case 7: // Теги для пос. талонов
            oReq.setRequestHeader ("OPERATION", "print_bp");
            break;
        case 9: // run_stat
            oReq.setRequestHeader ("OPERATION", "stat_srv");
            break;
        case 10: // CREWCHECKIN
            break;
        case 11: // Печать ПТ
            oReq.setRequestHeader ("OPERATION", "print_bp2");
            break;
        default:
            oReq.setRequestHeader ("OPERATION", "tlg_srv");
            break;
    }
    /*
    if(rowindex == 7) // если выбрана запрос 'Теги для пос. талонов'
        oReq.setRequestHeader ("OPERATION", "print_bp");
    else if(rowindex == 9) // если выбрана запрос run_stat
        oReq.setRequestHeader ("OPERATION", "stat_srv");
    else if(rowindex == 10) // если выбрана запрос 'Запрос в XML формате', вообще не клеим OPERATION
        ;
    else
        oReq.setRequestHeader ("OPERATION", "tlg_srv");
        */
    oReq.send(content);
  } else {
    var oField, sFieldType, nFile, sSearch = "";
    for (var nItem = 0; nItem < oFormElement.elements.length; nItem++) {
      oField = oFormElement.elements[nItem];
      if (!oField.hasAttribute("name")) { continue; }
      sFieldType = oField.nodeName.toUpperCase() === "INPUT" ? oField.getAttribute("type").toUpperCase() : "TEXT";
      if (sFieldType === "FILE") {
        for (nFile = 0; nFile < oField.files.length; sSearch += "&" + escape(oField.name) + "=" + escape(oField.files[nFile++].name));
      } else if ((sFieldType !== "RADIO" && sFieldType !== "CHECKBOX") || oField.checked) {
        sSearch += "&" + escape(oField.name) + "=" + escape(oField.value);
      }
    }
    oReq.open("get", oFormElement.action.replace(/(?:\?.*)?$/, sSearch.replace(/^&/, "?")), true);
    oReq.send(null);
  }
}

function get_select()
{
    return document.getElementById('destination');
}

function get_username()
{
    return conn_list[get_select().selectedIndex].username;
}

function get_password()
{
    return conn_list[get_select().selectedIndex].password;
}

function get_client_id()
{
    return conn_list[get_select().selectedIndex].client_id;
}

function get_action()
{
    return conn_list[get_select().selectedIndex].action;
}

function select_change() {
    document.getElementById('dst_login').innerHTML = get_username();
    document.getElementById('dst_pwd').innerHTML = get_password();
    document.getElementById('dst_client').innerHTML = get_client_id();
    document.getElementById('dst_addr').innerHTML = get_action();
    document.forms[0].action = get_action();
}

var table_selectedRows = new Array();
function table_onSelectMasterRow(node, tableNo, newMasterIndex) {
    table_onSelectRow(node, tableNo);
    document.getElementById('content').value = requests[newMasterIndex];
    document.getElementById('response').value = '';
}

function table_onSelectRow(node, tableNo) {    
    if (node.className != "currentrow") {
        var previousRow = table_selectedRows[tableNo];
        if (previousRow == null) {
            previousRow = node.parentNode.children[1];
        }
        requests[previousRow.rowIndex - 1] = document.getElementById('content').value;
        previousRow.className = "";
        var previousCells = previousRow.children;
        for (var pc = 0;pc < previousCells.length;pc++) {
            previousCells[pc].className = "";        
        }
        node.className = "currentrow";
        var nodeCells = node.children;
        for (var nc = 0;nc < nodeCells.length;nc++) {
            nodeCells[nc].className = "currentcell";        
        }
        table_selectedRows[tableNo] = node;
    }
}

window.onload = function()
{
    document.getElementById('content').value = requests[0];

    var item = document.createElement('option');

    for(var i = 0; i < conn_list.length; i++) {
        item.innerHTML = conn_list[i].descr;
        get_select().appendChild(item.cloneNode(true));
    }

    select_change();

    var matches = /(.*)\/.*$/.exec(window.location.pathname);
    /*
    if(matches[1] == '/astra-test') {
        conn.username =   'WEBSRV';
        conn.password =   'WEBSRVPWD';
        conn.client_id =  'WEBSRV';
        conn.action =     'http://wst.sirena-travel.ru/astra-test/';
    }
    */
};
