class Range {
    constructor(from, to) {
	this.from = from;
	this.to = to;
    }
};

class Ranges {
    constructor(seed) {
        if (seed === undefined) {
            this.ranges = [];
        } else {
            this.ranges = seed;
        }
    }

    set(num, state) {
	if (state) {
	    this.insert(num);
	} else {
	    this.remove(num);
	}
    }

    get(num) {
	for (var i = 0; i < this.ranges.length; i++) {
            var r = this.ranges[i];
	    if (num > r.to) {
		continue; // It's beyond r
	    }

	    // It's in or before r
	    return num >= r.from && num <= r.to;
	}
    }

    insert(num) {
	for (var i = 0; i < this.ranges.length; i++) {
	    var r = this.ranges[i];
	    if (num > r.to) {
		continue; // It's beyond r
	    }
	    if (num >= r.from) {
		return;  // It's already in r
	    }

	    // It's before r

	    if (i > 0) {
		var p = this.ranges[i-1];
		if (p.to + 1 == num) {
		    if (r.from - 1 == num) {
			p.to = r.to;
			this.ranges.splice(i, 1);
			return;
		    } else {
			p.to++;
			return;
		    }
		}
	    }

	    if (r.from - 1 == num) {
		r.from--;
		return;
	    }

	    this.ranges.splice(i, 0, new Range(num, num));
	    return;
	}

	if (this.ranges.length > 0) {
	    var r = this.ranges[this.ranges.length-1];
	    if (r.to + 1 == num) {
		r.to++;
		return;
	    }
	}
	this.ranges.push(new Range(num, num));
    }

    remove(num) {
	for (var i = 0; i < this.ranges.length; i++) {
	    var r = this.ranges[i];
	    if (num > r.to) {
		continue; // It's beyond r
	    }
	    if (num < r.from) {
		return;  // It's not set
	    }

	    if (num == r.from) {
		if (r.from == r.to) {
		    this.ranges.splice(i, 1);
		    return;
		}
		r.from++;
		return;
	    } else if (num == r.to) {
		r.to--;
		return;
	    }

	    this.ranges.splice(i, 1,
			       new Range(r.from, num-1),
			       new Range(num+1, r.to));
	    return;
	}
    }

    toString() {
	if (this.ranges.length == 0) {
	    return "empty";
	}

	var out = [];
	for (r of this.ranges) {
	    if (r.from == r.to) {
		out.push(r.from);
	    } else {
		out.push(r.from + "-" + r.to);
	    }
	}
	return out.join(", ");
    }
};

function testRangeInsert() {
    const equalsCheck = (a, b, msg) => {
	var want = JSON.stringify(a);
	var got = JSON.stringify(b);
	if (want !== got) {
	    console.log("FAIL: " + msg + " want " + want + " got " + got);
	}
    };

    var testCases = [
	{in: [3], want: [new Range(3,3)]},
	{in: [3,2], want: [new Range(2,3)]},
	{in: [3,1], want: [new Range(1,1),new Range(3,3)]},
	{in: [3,1,2], want: [new Range(1,3)]},
	{in: [3,4], want: [new Range(3,4)]},
	{in: [3,10,5], want: [new Range(3,3), new Range(5,5), new Range(10,10)]},
    ];

    for (var tc of testCases) {
	console.log("test case " + JSON.stringify(tc));

	r = new Ranges();
	for (var v of tc.in) {
	    r.insert(v);
	}
	equalsCheck(tc.want, r.ranges);
    }
}

function testRangeRemove() {
    const equalsCheck = (a, b, msg) => {
	var want = JSON.stringify(a);
	var got = JSON.stringify(b);
	if (want !== got) {
	    console.log("FAIL: " + msg + " want " + want + " got " + got);
	}
    };

    var testCases = [
	{start: [new Range(3,3)], remove: 3, want: []},
	{start: [new Range(3,4)], remove: 3, want: [new Range(4,4)]},
	{start: [new Range(3,4)], remove: 4, want: [new Range(3,3)]},
	{start: [new Range(3,5)], remove: 4, want: [new Range(3,3), new Range(5,5)]},
	{start: [new Range(3,3), new Range(5,5), new Range(7,7)], remove: 5,
	 want: [new Range(3,3), new Range(7,7)]},
    ];

    for (var tc of testCases) {
	console.log("test case " + JSON.stringify(tc));

	r = new Ranges();
	r.ranges = tc.start;
	r.remove(tc.remove);
	equalsCheck(tc.want, r.ranges);
    }
}

testRangeInsert();
testRangeRemove();

curElem = document.getElementById("curElem");
curValElem = document.getElementById("curVal");

MODE_NAV = 0;
MODE_ON = 1;
MODE_OFF = 2;
MODE_FIND = 3;

upButtonElem = document.getElementById("up");
downButtonElem = document.getElementById("down");
ffUpButtonElem = document.getElementById("ffUp");
ffDownButtonElem = document.getElementById("ffDown");
rangeElem = document.getElementById("position");

statusElem = document.getElementById("status");
actionButtonElem = document.getElementById("action");
actionLabelElem = document.getElementById("actionLabel");

function keyDown(event) {
    switch (event.key) {
    case "ArrowRight":
	return upClicked();
    case "ArrowLeft":
	return downClicked();
    case " ":
	return actionClicked();
    }
}

function upClicked(n = 1) {
    if (curLight == maxLight) {
	return;
    }
    var next = curLight + n;
    if (next > maxLight) {
	next = maxLight;
    }

    if (actionMode == MODE_ON || actionMode == MODE_OFF) {
	state.set(curLight, actionMode == MODE_ON);
    }

    curLight = next;
    rangeElem.value = next;
    update();
}

function downClicked(n = 1) {
    if (curLight == minLight) {
	return;
    }
    var next = curLight - n;
    if (next < minLight) {
	next = minLight;
    }

    if (actionMode == MODE_ON || actionMode == MODE_OFF) {
	state.set(curLight, actionMode == MODE_ON);
    }

    curLight = next;
    rangeElem.value = next;
    update();
}

function ffUpClicked() { upClicked(10); }
function ffDownClicked() { downClicked(10); }

function rangeInput(event) {
    if (actionMode == MODE_ON || actionMode == MODE_OFF) {
	state.set(curLight, actionMode == MODE_ON);
    }
    actionMode = MODE_FIND;

    curLight = Number(rangeElem.value);
    update(false);  // low priority
}

function rangeChange(event) {
    if (actionMode == MODE_ON || actionMode == MODE_OFF) {
	state.set(curLight, actionMode == MODE_ON);
    }
    actionMode = MODE_NAV;

    curLight = Number(rangeElem.value);
    update();
}

function actionClicked() {
    var next = MODE_NAV;
    switch (actionMode) {
    case MODE_NAV:  next = MODE_ON;  break;
    case MODE_ON:   next = MODE_OFF; break;
    case MODE_OFF:  next = MODE_NAV; break;
    case MODE_FIND: next = MODE_NAV; break;
    }

    actionMode = next;
    update();
}

function statusClicked() {
    saveContent();
}

function actionModeToString(actionMode) {
    switch (actionMode) {
    case MODE_NAV:
	return "NAV";
    case MODE_ON:
	return "ON";
    case MODE_OFF:
	return "OFF";
    case MODE_FIND:
        return "FIND";
    default:
	return "???";
    }
}

function update(highPriority) {
    updateStatus();
    updateServer(highPriority);
}

function updateStatus() {
    curElem.textContent = curLight;
    curValElem.textContent = state.get(curLight) ? "ON" : "OFF";
    actionLabelElem.textContent = actionModeToString(actionMode);
}

function updateServer(highPriority) {
    sendToServer("/set", highPriority);
}

function saveContent() {
    sendToServer("/save", true);
}

outstandingRequests = 0;

function sendToServer(method, isHighPri) {
    if (isHighPri === false && outstandingRequests > 0) {
        console.log("ignoring");
        return;
    }

    var data = {
        Metadata: {
            CurLight: curLight,
            Mode: actionModeToString(actionMode),
        },
        OnRanges: state.ranges,
    };

    var req = new XMLHttpRequest();
    req.open("POST", method);
    req.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
    req.onloadend = function() { outstandingRequests--; }

    outstandingRequests++;
    req.send(JSON.stringify(data));
}

// Initialize state

state = new Ranges(seed);

curLight = minLight;
rangeElem.value = curLight;
actionMode = MODE_NAV;
update();

// Add listeners. We don't want to respond to anything until state is
// initialized.

document.addEventListener("keydown", keyDown);

upButtonElem.addEventListener("click", (event) => upClicked());
downButtonElem.addEventListener("click", (event) => downClicked());
ffUpButtonElem.addEventListener("click", (event) => ffUpClicked());
ffDownButtonElem.addEventListener("click", (event) => ffDownClicked());

rangeElem.addEventListener("input", rangeInput);
rangeElem.addEventListener("change", rangeChange);

actionButtonElem.addEventListener("click", actionClicked);
statusElem.addEventListener("click", statusClicked);
