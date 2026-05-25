const scr = eos.view.active();

const face = new lv.image(scr);
face.src = "watchface.bin";
face.center();

const hour = eos.clockHand.create(scr, "clock_hand.bin", eos.CLOCK_HAND_HOUR, 10, 176);
const minute = eos.clockHand.create(scr, "clock_hand.bin", eos.CLOCK_HAND_MINUTE, 10, 176);
const second = eos.clockHand.create(scr, "clock_hand.bin", eos.CLOCK_HAND_SECOND, 10, 176);

eos.clockHand.center(hour);
eos.clockHand.center(minute);
eos.clockHand.center(second);

hour.scale = 180;
minute.scale = 220;
second.scale = 245;
