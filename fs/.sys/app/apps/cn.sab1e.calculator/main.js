let view = eos.view.active();

let currentActivity = eos.activity.current();
eos.activity.setAppHeaderVisible(currentActivity, false);

const BUTTON_SCALE_PRESSED = 350;

const BACKSPACE_UNICODE = "\uE219";
const DIVIDE_UNICODE = "\uE23E";
const MULTIPLY_UNICODE = "\uE197";
const SUBTRACT_UNICODE = "\uE7AD";

const KEY_LAYOUT = [
    { key: "AC", type: "func", row: 0, col: 0, span: 1 },
    { key: "+/-", type: "func", row: 0, col: 1, span: 1 },
    { key: "%", type: "func", row: 0, col: 2, span: 1 },
    { key: DIVIDE_UNICODE, type: "op", row: 0, col: 3, span: 1 },
    { key: "7", type: "digit", row: 1, col: 0, span: 1 },
    { key: "8", type: "digit", row: 1, col: 1, span: 1 },
    { key: "9", type: "digit", row: 1, col: 2, span: 1 },
    { key: MULTIPLY_UNICODE, type: "op", row: 1, col: 3, span: 1 },
    { key: "4", type: "digit", row: 2, col: 0, span: 1 },
    { key: "5", type: "digit", row: 2, col: 1, span: 1 },
    { key: "6", type: "digit", row: 2, col: 2, span: 1 },
    { key: SUBTRACT_UNICODE, type: "op", row: 2, col: 3, span: 1 },
    { key: "1", type: "digit", row: 3, col: 0, span: 1 },
    { key: "2", type: "digit", row: 3, col: 1, span: 1 },
    { key: "3", type: "digit", row: 3, col: 2, span: 1 },
    { key: "+", type: "op", row: 3, col: 3, span: 1 },
    { key: "0", type: "digit", row: 4, col: 0, span: 2 },
    { key: ".", type: "digit", row: 4, col: 2, span: 1 },
    { key: "=", type: "op", row: 4, col: 3, span: 1 },
];

const COLOR_BG = lv.color.hex(0x000000);
const COLOR_DISPLAY = lv.color.hex(0x1C1C1E);
const COLOR_DIGIT = lv.color.hex(0x2C2C2E);
const COLOR_FUNC = lv.color.hex(0xA5A5A5);
const COLOR_OP = lv.color.hex(0xFF9F0A);
const COLOR_TEXT_LIGHT = lv.color.hex(0xFFFFFF);
const COLOR_TEXT_DARK = lv.color.hex(0x000000);
const MAX_INPUT_LEN = 14;
const MAX_ABS_VALUE = 999999999999;
const STATE_PRESSED = 0x0020;

let currentInput = "0";
let storedValue = null;
let pendingOperator = null;
let isResultShown = false;
let hasError = false;
let errorText = "Error";

let displayLabel = null;
let displayRightInset = 10;
let deleteButton = null;
let opButtons = {};
let activeOpKey = null;
let displayDimmed = true;

function resetAll() {
    currentInput = "0";
    storedValue = null;
    pendingOperator = null;
    isResultShown = false;
    hasError = false;
    errorText = "Error";
    clearActiveOp();
}

function setError(text) {
    hasError = true;
    errorText = text || "Error";
}

function isValueOverflow(value) {
    if (!isFinite(value)) return true;
    return Math.abs(value) > MAX_ABS_VALUE;
}

function isTextOverflow(text) {
    let t = text;
    if (t[0] === SUBTRACT_UNICODE) t = t.slice(1);
    return t.length > MAX_INPUT_LEN;
}

function isNextInputAllowed(nextText) {
    if (isTextOverflow(nextText)) return false;

    let n = parseFloat(nextText);
    if (isNaN(n)) return true;
    return !isValueOverflow(n);
}

function formatNumber(value) {
    if (isValueOverflow(value)) return null;

    let rounded = parseFloat(value.toFixed(10));
    let text = String(rounded);
    if (isTextOverflow(text)) return null;
    return text;
}

function getCurrentNumber() {
    let n = parseFloat(currentInput);
    if (isNaN(n)) return 0;
    return n;
}

function applyOperator(a, b, op) {
    if (op === "+") return a + b;
    if (op === SUBTRACT_UNICODE) return a - b;
    if (op === MULTIPLY_UNICODE) return a * b;
    if (op === DIVIDE_UNICODE) {
        if (b === 0) return null;
        return a / b;
    }
    return b;
}

function updateDisplay() {
    if (!displayLabel) return;
    displayLabel.text = hasError ? errorText : currentInput;
    displayLabel.align(lv.ALIGN_RIGHT_MID, -displayRightInset, 0);
    let shouldDim = !hasError && (currentInput === "0" || (isResultShown && pendingOperator !== null));
    setDisplayDimmed(shouldDim);
}

function backspaceInput() {
    if (hasError) {
        resetAll();
        return;
    }

    if (isResultShown) {
        isResultShown = false;
    }

    if (currentInput.length <= 1) {
        currentInput = "0";
        return;
    }

    if (currentInput.length === 2 && currentInput[0] === SUBTRACT_UNICODE) {
        currentInput = "0";
        return;
    }

    currentInput = currentInput.slice(0, -1);
}

function setDisplayDimmed(dimmed) {
    if (!displayLabel) return;
    if (displayDimmed === dimmed) return;
    displayDimmed = dimmed;

    displayLabel.setStyleTranslateX(dimmed ? 20 : 0, lv.PART_MAIN);

    if (!deleteButton) return;

    deleteButton.removeFlag(lv.OBJ_FLAG_CLICKABLE);
    if (!dimmed) {
        deleteButton.addFlag(lv.OBJ_FLAG_CLICKABLE);
    }
    deleteButton.setStyleOpa(dimmed ? 0 : 255, lv.PART_MAIN);
}

function restoreOpButton(key) {
    let btn = opButtons[key];
    if (!btn) return;
    btn.setStyleBgColor(COLOR_OP, lv.PART_MAIN);
    let lbl = btn.getChild(0);
    if (lbl) lbl.setStyleTextColor(COLOR_TEXT_LIGHT, lv.PART_MAIN);
}

function setActiveOp(key) {
    if (activeOpKey !== null && activeOpKey !== key) {
        restoreOpButton(activeOpKey);
    }
    activeOpKey = key;
    let btn = opButtons[key];
    if (!btn) return;
    btn.setStyleBgColor(COLOR_TEXT_LIGHT, lv.PART_MAIN);
    let lbl = btn.getChild(0);
    if (lbl) lbl.setStyleTextColor(COLOR_OP, lv.PART_MAIN);
}

function clearActiveOp() {
    if (activeOpKey === null) return;
    restoreOpButton(activeOpKey);
    activeOpKey = null;
}

function inputDigit(digit) {
    if (hasError) resetAll();

    if (isResultShown) {
        if (!isNextInputAllowed(digit)) return;
        currentInput = digit;
        isResultShown = false;
        return;
    }

    let nextText = currentInput === "0" ? digit : currentInput + digit;
    if (!isNextInputAllowed(nextText)) {
        setError("Overflow");
        return;
    }

    currentInput = nextText;
}

function inputDot() {
    if (hasError) resetAll();

    if (isResultShown) {
        if (!isNextInputAllowed("0.")) return;
        currentInput = "0.";
        isResultShown = false;
        return;
    }

    if (currentInput.indexOf(".") >= 0) return;

    let nextText = currentInput + ".";
    if (!isNextInputAllowed(nextText)) {
        setError("Overflow");
        return;
    }

    currentInput = nextText;
}

function toggleSign() {
    if (hasError) return;
    if (currentInput === "0") return;
    if (currentInput[0] === SUBTRACT_UNICODE) {
        currentInput = currentInput.slice(1);
    } else {
        currentInput = SUBTRACT_UNICODE + currentInput;
    }
}

function toPercent() {
    if (hasError) return;
    let value = getCurrentNumber() / 100;
    let text = formatNumber(value);
    if (text === null) {
        setError("Overflow");
        return;
    }

    currentInput = text;
    isResultShown = true;
}

function pressOperator(op) {
    if (hasError) return;

    let currentValue = getCurrentNumber();

    if (storedValue === null) {
        storedValue = currentValue;
    } else if (pendingOperator && !isResultShown) {
        let result = applyOperator(storedValue, currentValue, pendingOperator);
        if (result === null) {
            setError("Error");
            return;
        }

        let text = formatNumber(result);
        if (text === null) {
            setError("Overflow");
            return;
        }

        storedValue = result;
        currentInput = text;
    }

    setActiveOp(op);
    pendingOperator = op;
    isResultShown = true;
}

function pressEqual() {
    if (hasError) return;
    if (!pendingOperator || storedValue === null) return;

    let currentValue = getCurrentNumber();
    let result = applyOperator(storedValue, currentValue, pendingOperator);
    if (result === null) {
        setError("Error");
        updateDisplay();
        return;
    }

    let text = formatNumber(result);
    if (text === null) {
        setError("Overflow");
        updateDisplay();
        return;
    }

    clearActiveOp();
    currentInput = text;
    storedValue = null;
    pendingOperator = null;
    isResultShown = true;
}

function handleKey(key) {
    if (key >= "0" && key <= "9") {
        inputDigit(key);
        updateDisplay();
        return;
    }

    if (key === ".") {
        inputDot();
        updateDisplay();
        return;
    }

    if (key === "AC") {
        resetAll();
        updateDisplay();
        return;
    }

    if (key === "+/-") {
        toggleSign();
        updateDisplay();
        return;
    }

    if (key === "%") {
        toPercent();
        updateDisplay();
        return;
    }

    if (key === "=") {
        pressEqual();
        updateDisplay();
        return;
    }

    pressOperator(key);
    updateDisplay();
}

function styleButton(btn, type, radius) {
    btn.setStyleBorderWidth(0, lv.PART_MAIN);
    btn.setStyleRadius(radius, lv.PART_MAIN);
    btn.setStyleShadowWidth(0, lv.PART_MAIN);

    if (type === "digit") {
        btn.setStyleBgColor(COLOR_DIGIT, lv.PART_MAIN);
        return;
    }

    if (type === "func") {
        btn.setStyleBgColor(COLOR_FUNC, lv.PART_MAIN);
        return;
    }

    btn.setStyleBgColor(COLOR_OP, lv.PART_MAIN);
}

function styleButtonText(label, type) {
    if (type === "func") {
        label.setStyleTextColor(COLOR_TEXT_DARK, lv.PART_MAIN);
        return;
    }
    label.setStyleTextColor(COLOR_TEXT_LIGHT, lv.PART_MAIN);
}

function bindKeyClick(btn, key) {
    btn.addEventCb(function () {
        handleKey(key);
    }, lv.EVENT_CLICKED, null);
}

function createUi() {
    let sw = eos.DISPLAY_WIDTH;
    let sh = eos.DISPLAY_HEIGHT;

    let pad = Math.max(8, Math.floor(sw * 0.04));
    let gap = Math.max(6, Math.floor(sw * 0.02));
    let displayH = Math.floor(sh * 0.28);

    let root = new lv.obj(view);
    root.setSize(sw, sh);
    root.setPos(0, 0);
    root.setStyleBgColor(COLOR_BG, lv.PART_MAIN);
    root.setStyleBorderWidth(0, lv.PART_MAIN);
    root.setStyleRadius(0, lv.PART_MAIN);
    root.setStylePadAll(0, lv.PART_MAIN);

    let displayWrap = new lv.obj(root);
    displayWrap.setPos(pad, pad);
    displayWrap.setSize(sw - pad * 2, displayH - pad);
    displayWrap.setStyleBgColor(COLOR_DISPLAY, lv.PART_MAIN);
    displayWrap.setStyleBorderWidth(0, lv.PART_MAIN);
    displayWrap.setStyleRadius(Math.floor((displayH - pad) / 3), lv.PART_MAIN);
    displayWrap.setStylePadAll(0, lv.PART_MAIN);

    let displayWrapW = sw - pad * 2;
    let displayWrapH = displayH - pad;
    let deleteBtnSize = Math.max(28, Math.floor(displayWrapH * 0.46));

    let deleteBtn = new lv.button(displayWrap);
    deleteBtn.setSize(deleteBtnSize, deleteBtnSize);
    deleteBtn.align(lv.ALIGN_RIGHT_MID, -8, 0);
    deleteBtn.setStyleRadius(Math.floor(deleteBtnSize / 2), lv.PART_MAIN);
    deleteBtn.setStyleBorderWidth(0, lv.PART_MAIN);
    deleteBtn.setStyleTextColor(COLOR_TEXT_DARK, lv.PART_MAIN);
    deleteBtn.setStyleBgOpa(0, lv.PART_MAIN);
    deleteBtn.setStyleAnimDuration(180, lv.PART_MAIN);
    deleteBtn.setStyleOpa(0, lv.PART_MAIN);
    deleteBtn.removeFlag(lv.OBJ_FLAG_CLICKABLE);
    deleteButton = deleteBtn;

    let deleteLabel = new lv.label(deleteBtn);
    deleteLabel.text = BACKSPACE_UNICODE;
    deleteLabel.center();

    deleteBtn.addEventCb(function () {
        backspaceInput();
        updateDisplay();
    }, lv.EVENT_CLICKED, null);

    displayRightInset = deleteBtnSize + 18;

    displayLabel = new lv.label(displayWrap);
    displayLabel.setWidth(displayWrapW - displayRightInset - 8);
    displayLabel.align(lv.ALIGN_RIGHT_MID, -displayRightInset, 0);
    displayLabel.setStyleTextColor(COLOR_TEXT_LIGHT, lv.PART_MAIN);
    displayLabel.setStyleTextAlign(lv.TEXT_ALIGN_RIGHT, lv.PART_MAIN);
    displayLabel.setFontSize(eos.FONT_SIZE_LARGE);
    displayLabel.setStyleAnimDuration(180, lv.PART_MAIN);
    displayLabel.text = "0";
    displayLabel.setStyleTranslateX(20, lv.PART_MAIN);


    let keypadY = displayH + pad;
    let keypadH = sh - keypadY - pad;
    let cellW = Math.floor((sw - pad * 2 - gap * 3) / 4);
    let cellH = Math.floor((keypadH - gap * 4) / 5);
    let radius = Math.floor(Math.min(cellW, cellH) / 2);

    for (let i = 0; i < KEY_LAYOUT.length; i++) {
        let item = KEY_LAYOUT[i];
        let span = item.span || 1;
        let x = pad + item.col * (cellW + gap);
        let y = keypadY + item.row * (cellH + gap);
        let w = cellW * span + gap * (span - 1);

        let btn = new lv.button(root);
        btn.setPos(x, y);
        btn.setSize(w, cellH);
        styleButton(btn, item.type, radius);

        let txt = new lv.label(btn);
        txt.text = item.key;
        txt.center();
        styleButtonText(txt, item.type);

        if (item.type === "op") {
            opButtons[item.key] = btn;
        }

        btn.setStyleTransformScale(256, lv.PART_MAIN);
        btn.setStyleTransformScale(490, lv.PART_MAIN | STATE_PRESSED);
        btn.setStyleAnimDuration(120, lv.PART_MAIN);

        let pivX = Math.floor(w / 2);
        let pivY = Math.floor(cellH / 2);
        if (item.col === 0) {
            pivX = 0;
        } else if (item.col + item.span >= 4) {
            pivX = w;
        }
        if (item.row === 0) {
            pivY = 0;
        } else if (item.row === 4) {
            pivY = cellH;
        }
        btn.setStyleTransformPivotX(pivX, lv.PART_MAIN);
        btn.setStyleTransformPivotY(pivY, lv.PART_MAIN);

        (function (b) {
            b.addEventCb(function () { b.moveForeground(); }, lv.EVENT_PRESSED, null);
        })(btn);

        bindKeyClick(btn, item.key);
    }

    updateDisplay();
}

createUi();
eos.console.log("[calculator] app started");
