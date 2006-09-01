function showAll() {
  // for now, assume only 1 table
  tbl = document.getElementsByTagName("table")[0];
  bdy = tbl.getElementsByTagName("tbody")[0];
  rows = bdy.getElementsByTagName("tr");
  for(i=0;i<rows.length;i++) {
//    rows[i].style.display="table-row";
    if(i % 2) { rows[i].className='entry0'; }
    else { rows[i].className='entry1'; }
  }
}

function checkQuery() {
  s = queryVariable("searchText");
  if(s=="") {
    return;
  }
  doSearch(s);
}

function searchRows() {
  s = document.getElementById("searchText").value;
  if(s=="") {
    showAll();
    return;
  }
  doSearch(s);
}

function doSearch(s) {
  re = new RegExp(s,"i")
  tbl = document.getElementsByTagName("table")[0];
  bdy = tbl.getElementsByTagName("tbody")[0];
  rows = bdy.getElementsByTagName("tr");
  j = 0;
  for(i=0; i<rows.length; i++) {
    s = ts_getInnerText(rows[i]);
    if(re.test(s)) {
//      rows[i].style.display="table-row";
      if(j % 2) { rows[i].className='entry1'; }
      else { rows[i].className='entry0'; }
      j++;
    } else {
//      rows[i].style.display="none";
      // for msie
      rows[i].className='hidden';
    }
  }
}

var SORT_COLUMN_INDEX;

function sortables_init() {
    // Find all tables with class sortable and make them sortable
    if (!document.getElementsByTagName) return;
    tbls = document.getElementsByTagName("table");
    for (ti=0;ti<tbls.length;ti++) {
        thisTbl = tbls[ti];
        if (((' '+thisTbl.className+' ').indexOf("sortable") != -1) && (thisTbl.id)) {
            ts_makeSortable(thisTbl);
        }
    }
}

function ts_makeSortable(table) {
    if (table.rows && table.rows.length > 0) {
        var firstRow = table.rows[0];
    }
    if (!firstRow) return;
    // We have a first row: assume it's the header, and make its contents clickable links
    for (var i=0;i<firstRow.cells.length;i++) {
        var cell = firstRow.cells[i];
        var txt = ts_getInnerText(cell);
        cell.innerHTML = '<a href="#" class="sortheader" onclick="ts_resortTable(this);return false;">'+txt+'</a>';
    }
}

function ts_getInnerText(el) {
	if (typeof el == "string") return el;
	if (typeof el == "undefined") { return el };
	if (el.innerText) return el.innerText; //Not needed but it is faster
	var str = "";
	
	var cs = el.childNodes;
	var l = cs.length;
	for (var i = 0; i < l; i++) {
		switch (cs[i].nodeType) {
			case 1: //ELEMENT_NODE
				str += ts_getInnerText(cs[i]);
				break;
			case 3: //TEXT_NODE
				str += cs[i].nodeValue;
				break;
		}
	}
	return str;
}

function ts_resortTable(lnk) {
    var td = lnk.parentNode;
    var column = td.cellIndex;
    var table = getParent(td,'TABLE');

    // Work out a type for the column
    if (table.rows.length <= 1) return;
    var sortType = COL_SORT_ARRAY[column];
    var sortfn = ts_sort_caseinsensitive;
    if (sortType == 1) sortfn = ts_sort_numeric;
    else if (sortType == 2) sortfn = ts_sort_date;

    var newRows = new Array();
    for (i=0;i<table.tBodies[0].rows.length;i++) { newRows[i] = table.tBodies[0].rows[i]; }

    SORT_COLUMN_INDEX = column;
    newRows.sort(sortfn);

    if (lnk.getAttribute("sortdir") == 'down') {
      newRows.reverse();
      lnk.setAttribute('sortdir','up');
    } else {
      lnk.setAttribute('sortdir','down');
    }

    // We appendChild rows that already exist to the tbody, so it moves them rather than creating new ones
    for (i=0;i<newRows.length;i++) {
       if(i % 2) { newRows[i].className='entry0'; }
       else { newRows[i].className='entry1'; }
       table.tBodies[0].appendChild(newRows[i]);
    }
}

function getParent(el, pTagName) {
	if (el == null) return null;
	else if (el.nodeType == 1 && el.tagName.toLowerCase() == pTagName.toLowerCase())	// Gecko bug, supposed to be uppercase
		return el;
	else
		return getParent(el.parentNode, pTagName);
}
function ts_sort_date(a,b) {
    // y2k notes: two digit years less than 50 are treated as 20XX, greater than 50 are treated as 19XX
    aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]);
    bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]);
    if (aa.length == 10) {
        dt1 = aa.substr(6,4)+aa.substr(3,2)+aa.substr(0,2);
    } else {
        yr = aa.substr(6,2);
        if (parseInt(yr) < 50) { yr = '20'+yr; } else { yr = '19'+yr; }
        dt1 = yr+aa.substr(3,2)+aa.substr(0,2);
    }
    if (bb.length == 10) {
        dt2 = bb.substr(6,4)+bb.substr(3,2)+bb.substr(0,2);
    } else {
        yr = bb.substr(6,2);
        if (parseInt(yr) < 50) { yr = '20'+yr; } else { yr = '19'+yr; }
        dt2 = yr+bb.substr(3,2)+bb.substr(0,2);
    }
    if (dt1==dt2) return 0;
    if (dt1<dt2) return -1;
    return 1;
}

function ts_sort_currency(a,b) { 
    aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]).replace(/[^0-9.]/g,'');
    bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]).replace(/[^0-9.]/g,'');
    return parseFloat(aa) - parseFloat(bb);
}

function ts_sort_numeric(a,b) { 
    aa = parseFloat(ts_getInnerText(a.cells[SORT_COLUMN_INDEX]));
    if (isNaN(aa)) aa = 0;
    bb = parseFloat(ts_getInnerText(b.cells[SORT_COLUMN_INDEX])); 
    if (isNaN(bb)) bb = 0;
    return aa-bb;
}

function ts_sort_caseinsensitive(a,b) {
    aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]).toLowerCase();
    bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]).toLowerCase();
    if(aa.localeCompare) return aa.localeCompare(bb);
    return order(aa,bb);
}

function ts_sort_default(a,b) {
    aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]);
    bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]);
    if(aa.localeCompare) return aa.localeCompare(bb);
    return order(aa,bb);
}

function order(aa,bb) {
  if (aa==bb) return 0;
  if (aa<bb) return -1;
  return 1;
}

function addEvent(elm, evType, fn, useCapture) {
// addEvent and removeEvent
// cross-browser event handling for IE5+,  NS6 and Mozilla
// By Scott Andrew
  if (elm.addEventListener) {
    elm.addEventListener(evType, fn, useCapture);
    return true;
  } else if (elm.attachEvent) {
    var r = elm.attachEvent("on"+evType, fn);
    return r;
  }
}

function queryVariable(variable) {
  var query = window.location.search.substring(1);
  var vars = query.split("&");
  for (var i=0;i<vars.length;i++) {
    var pair = vars[i].split("=");
    if (pair[0] == variable) {
      return pair[1];
    }
  }
  return "";
}

// this table sorting script is modified from http://www.kryogenix.org/code/browser/sorttable/
// released under the MIT license
addEvent(window, "load", sortables_init);
addEvent(window, "load", checkQuery);
