void dcb_sasi_process_command() {
  if ((dcb_sasist & 0xF8) == 0x58 && (dcb_sasi_cmdbuf[1] >> 5) > 0) {
    // If LUN > 0, bail out now, saves repeating it in every actual command
    dcb_sasi_return_status(0x02, 0x22, 0);
    return;
  }

  switch (dcb_sasi_cmdbuf[0]) {
    case 0x00: dcb_sasi_return_status(0, 0, 0);       break; // TEST UNIT READY
    case 0x03: dcb_sasicmd_request_sense();           break; // REQUEST SENSE
    // case 0x08: dcb_sasicmd_read6();                   break; // READ(6)
    // case 0x0A: dcb_sasi_return_status(0x02, 0x03, 0); break; // WRITE(6)
    case 0x12: dcb_sasicmd_inquiry();                 break; // INQUIRY
    default:   dcb_sasi_return_status(0x02, 0x20, 0);
  }
}

static void memcpyv(volatile uint8_t *dest, volatile const uint8_t *src, uint cnt) {
  while (cnt--) { *dest++ = *src++; }
}

static void dcb_sasicmd_request_sense() {
  switch (dcb_sasist & 0xF8) {
    case 0x58: // Busy, following output command
      dcb_sasi_remain = min(dcb_sasi_cmdbuf[4], sizeof dcb_sasi_sensebuf);
      memcpyv(dcb_sasi_databuf, dcb_sasi_sensebuf, dcb_sasi_remain);
      dcb_sasist = 0xB8;
      break;

    case 0x38: // Busy, following input data
      dcb_sasi_return_status(0, 0, 0);
      break;
  }
}

static void dcb_sasicmd_inquiry() {
  static const unsigned char dcb_sasi_inqbuf[36] = {
    0x03, 0x00, 0x00, 0x02, (sizeof dcb_sasi_inqbuf) - 5, 0x00, 0x00, 0x00,
    'D', 'A', 'V', 'I', 'D', 'K', 'N', 'O',
    'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', ' ', ' ', ' ',
    '0', '4', '2', '0'
  };

  switch (dcb_sasist & 0xF8) {
    case 0x58:
      dcb_sasi_remain = min(dcb_sasi_cmdbuf[4], sizeof dcb_sasi_inqbuf);
      memcpyv(dcb_sasi_databuf, dcb_sasi_inqbuf, dcb_sasi_remain);
      dcb_sasist = 0xB8;
      break;

    case 0x38:
      dcb_sasi_return_status(0, 0, 0);
      break;
  }
}
