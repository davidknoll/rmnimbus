static volatile uint8_t dcb_status  = 0x00;
static volatile uint8_t dcb_command = 0x00;
static volatile uint8_t dcb_track   = 0x00;
static volatile uint8_t dcb_sector  = 0x00;
static volatile uint8_t dcb_control = 0x00;

static volatile alarm_id_t dcb_alarm = 0;
static volatile bool dcb_stepdir = false;

CircularBuffer<uint8_t,1024> dcb_readbuf;
CircularBuffer<uint8_t,1024> dcb_writebuf;

static const uint dcb_stepdelays[] = { 3, 6, 10, 15 };

// 512 bytes test boot sector
// This is a copy of the boot sector from the welcome floppy
static const uint8_t dcb_testsector[512] = {
  0xE9, 0x18, 0x01, 0x52, 0x4D, 0x4C, 0x24, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x01, 0x00,
  0x02, 0x70, 0x00, 0xA0, 0x05, 0xF9, 0x03, 0x00, 0x09, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x02, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x49,
  0x4F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x53, 0x59, 0x53, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x20,
  0x20, 0x20, 0x53, 0x59, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53,
  0x51, 0x50, 0xB9, 0x02, 0x00, 0x8B, 0x1E, 0x65, 0x00, 0xA1, 0x67, 0x00, 0x89, 0x04, 0x58, 0xCD,
  0xF0, 0x59, 0x5B, 0x0A, 0xE4, 0xC3, 0x53, 0x52, 0xB8, 0x04, 0x00, 0x33, 0xDB, 0x03, 0x16, 0x75,
  0x00, 0x13, 0x1E, 0x77, 0x00, 0xBE, 0x69, 0x00, 0x89, 0x4C, 0x02, 0x89, 0x54, 0x04, 0x89, 0x5C,
  0x06, 0x89, 0x7C, 0x08, 0x8C, 0x44, 0x0A, 0xE8, 0xC5, 0xFF, 0x0A, 0xE4, 0x5A, 0x5B, 0xC3, 0x52,
  0x33, 0xD2, 0xF7, 0xF1, 0x0B, 0xD2, 0x74, 0x01, 0x40, 0x5A, 0xC3, 0x26, 0x8A, 0x05, 0x3C, 0x00,
  0x74, 0x06, 0x3C, 0x2E, 0x74, 0x02, 0x3C, 0xE5, 0xC3, 0x52, 0x57, 0xB1, 0x20, 0xF6, 0xE1, 0x03,
  0xF8, 0xE8, 0xE7, 0xFF, 0x74, 0x31, 0xB9, 0x0B, 0x00, 0x57, 0xFC, 0xF3, 0xA6, 0x5F, 0x75, 0x27,
  0xBE, 0x69, 0x00, 0x26, 0x8B, 0x45, 0x1C, 0x8B, 0x0E, 0x0B, 0x00, 0xE8, 0xC1, 0xFF, 0x8B, 0xC8,
  0x26, 0x8B, 0x45, 0x1A, 0x2D, 0x02, 0x00, 0x32, 0xF6, 0x8A, 0x16, 0x0D, 0x00, 0xF7, 0xE2, 0x03,
  0x06, 0x79, 0x00, 0x3A, 0xC0, 0xEB, 0x01, 0xF9, 0x5F, 0x5A, 0xC3, 0x8B, 0xF8, 0x8C, 0xC8, 0x8E,
  0xD8, 0x8C, 0xD0, 0xA3, 0x7B, 0x00, 0x89, 0x26, 0x7D, 0x00, 0x8C, 0xC8, 0x2D, 0x20, 0x00, 0x8E,
  0xC0, 0x2D, 0x20, 0x00, 0x8E, 0xD0, 0xBC, 0x00, 0x02, 0x8B, 0xC7, 0x50, 0x53, 0x51, 0x52, 0xA3,
  0xF7, 0x01, 0x89, 0x0E, 0x65, 0x00, 0x89, 0x16, 0x67, 0x00, 0x33, 0xC0, 0xA3, 0x75, 0x00, 0x8A,
  0x0E, 0x0A, 0x00, 0x80, 0xF9, 0xFF, 0x75, 0x08, 0x8B, 0x04, 0xA3, 0x75, 0x00, 0x8B, 0x44, 0x02,
  0xA3, 0x77, 0x00, 0xA1, 0x0B, 0x00, 0xB1, 0x05, 0xD3, 0xE8, 0x8B, 0xC8, 0xA1, 0x11, 0x00, 0xE8,
  0x4D, 0xFF, 0xA3, 0x79, 0x00, 0xA0, 0x16, 0x00, 0xF6, 0x26, 0x10, 0x00, 0x03, 0x06, 0x0E, 0x00,
  0x8B, 0xD0, 0x01, 0x06, 0x79, 0x00, 0xB9, 0x01, 0x00, 0x33, 0xFF, 0xE8, 0x08, 0xFF, 0x75, 0x5B,
  0xE8, 0x38, 0xFF, 0x74, 0x56, 0x26, 0x8A, 0x65, 0x0B, 0x32, 0xC0, 0xF6, 0xC4, 0x08, 0x74, 0x02,
  0xFE, 0xC0, 0x8A, 0xD0, 0xBE, 0x4F, 0x00, 0xE8, 0x2F, 0xFF, 0x72, 0x3F, 0x92, 0xFE, 0xC0, 0xBE,
  0x5A, 0x00, 0xE8, 0x24, 0xFF, 0x72, 0x34, 0x2B, 0x06, 0x79, 0x00, 0x8B, 0xD8, 0x03, 0xC8, 0x53,
  0x33, 0xFF, 0xA1, 0xF7, 0x01, 0x8E, 0xC0, 0xE8, 0xCC, 0xFE, 0x5B, 0x75, 0x1E, 0xA1, 0x0B, 0x00,
  0xB1, 0x04, 0xD3, 0xE8, 0xF7, 0xE3, 0x03, 0x06, 0xF7, 0x01, 0x8B, 0xF0, 0xA0, 0x4E, 0x00, 0x32,
  0xE4, 0x8B, 0xF8, 0x5A, 0x59, 0x5B, 0x58, 0xFF, 0x2E, 0xF5, 0x01, 0xA1, 0x7B, 0x00, 0x8E, 0xD0,
  0x8B, 0x26, 0x7D, 0x00, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/**
 * To be called during sketch setup()
 */
void dcb_setup() {
  // Nothing here yet
}

/**
 * To be called during sketch loop()
 */
void dcb_loop() {
  // Nothing here yet
}

/**
 * Read from an emulated Disc Controller Board
 */
uint dcb_read(uint address) {
  uint data = 0x00;
  switch (address & 0x1E) {
    case 0x08: // FDC status
      digitalWrite(nINT0, HIGH);
      data = dcb_status;
      break;
    case 0x0A: // FDC track
      data = dcb_track;
      break;
    case 0x0C: // FDC sector
      data = dcb_sector;
      break;
    case 0x0E: // FDC data
      dcb_fdcclrdrq();
      data = dcb_readbuf.shift();
      dcb_alarm = add_alarm_in_us(16, dcb_callback_readbyte, nullptr, true);
      break;

    case 0x10: // SASI status
    case 0x12:
    case 0x14:
    case 0x16:
      if (dcb_control & 0x20) { data = 0x04; } // Motor on?
      break;
  }
  return data;
}

/**
 * Write to an emulated Disc Controller Board
 */
void dcb_write(uint address, uint data) {
  switch (address & 0x1E) {
    case 0x00: // FDC control
    case 0x02:
    case 0x04:
    case 0x06:
      dcb_control = data;
      break;
    case 0x08: // FDC command
      cancel_alarm(dcb_alarm);
      digitalWrite(nDMA,  HIGH);
      digitalWrite(nINT0, HIGH);
      dcb_command = data;
      dcb_docommand();
      break;
    case 0x0A: // FDC track
      dcb_track = data;
      break;
    case 0x0C: // FDC sector
      dcb_sector = data;
      break;
    case 0x0E: // FDC data
      dcb_fdcclrdrq();
      dcb_writebuf.push(data);
      break;
  }
}

static void dcb_docommand() {
  if (dcb_command & 0x80) {               // Type II/III/IV command
    if (dcb_command & 0x40) {             // Type III/IV command
      if ((dcb_command & 0x30) == 0x10) { // Type IV command
        dcb_type4();
      } else {                            // Type III command
        dcb_type3();
      }
    } else {                              // Type II command
      dcb_type2();
    }
  } else {                                // Type I command
    dcb_type1();
  }
}

static void dcb_type1() {
  uint8_t to_track;
  dcb_status = 0x41;

  if (dcb_command & 0x08) { dcb_status |= 0x20; }                  // Head load?
  if (dcb_command & 0x40) { dcb_stepdir = !(dcb_command & 0x20); } // Step direction

  if (dcb_command & 0x60) { // Step / step-in / step-out
    dcb_alarm = add_alarm_in_ms(dcb_stepdelays[dcb_command & 0x03], dcb_callback_step, nullptr, true);
  } else {                  // Seek / restore
    if (!(dcb_command & 0x10)) { dcb_writebuf.push(0); } // Restore
    to_track = dcb_writebuf.isEmpty() ? 0 : dcb_writebuf.last();

    if (dcb_track == to_track) {
      dcb_schedule_irqbusy();
    } else {
      dcb_alarm = add_alarm_in_ms(dcb_stepdelays[dcb_command & 0x03], dcb_callback_seek, nullptr, true);
    }
  }
}

static void dcb_type2() {
  dcb_status = 0x01;

  if (dcb_command & 0x04) {
    dcb_alarm = add_alarm_in_ms(15, dcb_callback_type2, nullptr, true);
  } else {
    dcb_alarm = add_alarm_in_ms( 0, dcb_callback_type2, nullptr, true);
  }
}

static void dcb_type3() {
  dcb_status = 0x01;

  if (dcb_command & 0x04) {
    dcb_alarm = add_alarm_in_ms(15, dcb_callback_type3, nullptr, true);
  } else {
    dcb_alarm = add_alarm_in_ms( 0, dcb_callback_type3, nullptr, true);
  }
}

static void dcb_type4() {
  if (dcb_status & 0x01) {
    dcb_status &= ~0x01;
  } else {
    dcb_status = 0x60;
    if (!dcb_track) { dcb_status |= 0x04; }
  }

  // Ready / not ready interrupts not yet implemented
  if (dcb_command & 0x4) { // Interrupt on every index pulse
    dcb_alarm = add_alarm_in_ms(200, dcb_callback_index, nullptr, true);
  }
  if (dcb_command & 0x8) { // Immediate interrupt
    digitalWrite(nINT0, LOW);
  }
}

static int64_t dcb_callback_index(alarm_id_t id, void *user_data) {
  if (dcb_control & 0x20) { // Motor on?
    dcb_status |= 0x02;     // Index pulse
    digitalWrite(nINT0, LOW);
  }
  return -200000;           // 3.5" at 300 rpm means 1 rev takes 0.2s
}

static void dcb_schedule_irqbusy() {
  if (dcb_track) { dcb_status &= ~0x04; } else { dcb_status |= 0x04; } // Track 0?
  if (dcb_command & 0x04) { // Verify?
    dcb_status |= 0x20;     // Head load
    dcb_alarm = add_alarm_in_ms(15, dcb_callback_irqbusy, nullptr, true);
  } else {
    dcb_alarm = add_alarm_in_ms( 0, dcb_callback_irqbusy, nullptr, true);
  }
}

static int64_t dcb_callback_irqbusy(alarm_id_t id, void *user_data) {
  dcb_status &= ~0x01;
  digitalWrite(nINT0, LOW);
  return 0;
}

static int64_t dcb_callback_seek(alarm_id_t id, void *user_data) {
  uint8_t to_track = dcb_writebuf.isEmpty() ? 0 : dcb_writebuf.last();
  if (dcb_track < to_track) { dcb_track++; } else if (dcb_track > to_track) { dcb_track--; }
  if (dcb_track != to_track) { return 1000 * dcb_stepdelays[dcb_command & 0x03]; }
  dcb_schedule_irqbusy();
  return 0;
}

static int64_t dcb_callback_step(alarm_id_t id, void *user_data) {
  if (dcb_command & 0x10) { // Update track
    if (dcb_stepdir) { dcb_track++; } else { dcb_track--; }
  }
  dcb_schedule_irqbusy();
  return 0;
}

static int64_t dcb_callback_type2(alarm_id_t id, void *user_data) {
  uint i;
  if (dcb_command & 0x20) { // Write sector
    dcb_status |= 0x40;     // Write protect
    dcb_callback_irqbusy(id, user_data);
  } else {                  // Read sector
    dcb_readbuf.clear();
    for (i = 0; i < sizeof dcb_testsector; i++) {
      dcb_readbuf.push(dcb_testsector[i]);
    }
    dcb_fdcsetdrq();
  }
  return 0;
}

static int64_t dcb_callback_type3(alarm_id_t id, void *user_data) {
  if (dcb_command & 0x10) {        // Write track
    dcb_status |= 0x40;            // Write protect
    dcb_callback_irqbusy(id, user_data);
  } else if (dcb_command & 0x20) { // Read track
    dcb_status  = 0xFC;            // Not implemented, just throw everything
    dcb_callback_irqbusy(id, user_data);
  } else {                         // Read address
    dcb_readbuf.clear();
    dcb_readbuf.push(dcb_track);
    dcb_readbuf.push((dcb_control & 0x10) ? 1 : 0); // Side
    dcb_readbuf.push(dcb_sector);
    dcb_readbuf.push(2);           // 512 bytes
    dcb_readbuf.push(0);           // Bogus CRC
    dcb_readbuf.push(0);
    dcb_fdcsetdrq();
  }
  return 0;
}

static int64_t dcb_callback_readbyte(alarm_id_t id, void *user_data) {
  uint i;
  if (dcb_status & 0x01) {
    if (dcb_readbuf.isEmpty()) {
      if ((dcb_command & 0xF0) == 0x90) { // Read sector multiple
        // Continue reading the next sector
        dcb_sector++;
        for (i = 0; i < sizeof dcb_testsector; i++) {
          dcb_readbuf.push(dcb_testsector[i]);
        }
        dcb_fdcsetdrq();
      } else {
        // Terminate the command
        dcb_callback_irqbusy(id, user_data);
      }
    } else {
      // There's more data
      dcb_fdcsetdrq();
    }
  }
  return 0;
}

static void dcb_fdcsetdrq() {
  dcb_status |=  0x02;
  if (dcb_control & 0x80) { digitalWrite(nDMA, LOW); }
}

static void dcb_fdcclrdrq() {
  dcb_status &= ~0x02;
  digitalWrite(nDMA, HIGH);
}