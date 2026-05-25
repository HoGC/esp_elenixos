// Get the active view
const view = eos.view.active();

// Create a button on the view
const button = new lv.button(view);
button.setSize(180, 64);
button.align(lv.ALIGN_CENTER, 0, 20);

// Add a label to the button
const label = new lv.label(button);
label.setText('Click Me');
label.center();

// Add an event callback to the button
button.addEventCb((e) => {
    eos.console.log('Button clicked!');
    label.setText(label.getText()=='Click Me'?'Clicked!':'Click Me');
}, lv.EVENT_CLICKED, null);
